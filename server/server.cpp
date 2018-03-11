#include "server.hpp"

GameServer::GameServer(json conf) {
  for (auto mapId : conf["maps"]) {
    loadMap(mapId);
  }

  m_server.init_asio();

  m_server.set_open_handler(bind(&GameServer::on_open, this, ::_1));
  m_server.set_close_handler(bind(&GameServer::on_close, this, ::_1));
  m_server.set_message_handler(bind(&GameServer::on_message, this, ::_1, ::_2));

  m_server.set_reuse_addr(true);
  int port = conf["port"];
  m_server.listen(port);
  m_server.start_accept();
  pushThread = std::thread(&GameServer::pushToClients, this);
  m_server.run();
}

GameServer::~GameServer() {
  pushingwholeSceneJson = false;
  pushThread.join();
  m_server.stop_listening();
}

std::string extractId(std::string sceneId) {
  std::string a = "player_";
  std::string id = sceneId.substr(a.length(), sceneId.length());
}

void GameServer::handleEvent(json event) {
  if (event["type"] == "switch map") {
    auto &session =
        *playerSessionsByName.find(extractId(event["sceneId"]))->second;
    session.status = "switching map";

    json switchCommand;
    switchCommand["command"] = "end scene";

    m_server.send(session.hdl, switchCommand.dump(),
                  websocketpp::frame::opcode::text);
  }
}

void GameServer::loadMap(std::string id) {
  auto last = new ServerSceneManager(&cache, id);

  last->scene.callback = [=](Object *object) {
    if (last->scene.isPlayer(object)) {
      playerCreated(extractId(object->sceneId));
    }
  };

  last->scene.externalEventHandler = [=](json event) { handleEvent(event); };

  sceneManagers.insert(std::make_pair(id, last));
}

void GameServer::on_open(connection_hdl hdl) {
  auto session = new PlayerSession;
  session->hdl = hdl;
  session->status = "not logged in";
  playerSessions.insert(std::make_pair(hdl, session));
}

void GameServer::on_close(connection_hdl hdl) {
  auto session_it = playerSessions.find(hdl);

  if (session_it == playerSessions.end()) {
    // TODO close connection
    return;
  }

  auto &session = *session_it->second;

  json removeEvent = json::parse(
      "{\"type\": \"remove\", \"sceneId\": \"player_" + session.id + "\" }");

  session.sceneManager->events.push_back(removeEvent);
  session.sceneManager->scene.submitEvents(json::array({removeEvent}));

  playerSessions.erase(session.hdl);
  playerSessionsByName.erase(session.id);

  session.sceneManager->playerSessions.erase(session.hdl);
  session.sceneManager->playerSessionsByName.erase(session.id);
}

void GameServer::on_message(connection_hdl hdl, server::message_ptr msg) {
  auto j = json::parse(msg->get_payload());

  auto session_it = playerSessions.find(hdl);

  if (session_it == playerSessions.end()) {
    // TODO close connection
    return;
  }

  auto &session = *session_it->second;

  // TODO correct SM
  if (j["command"] == "login") {
    session.id = j["id"];
    session.status = "not initialized";

    auto const &playerDefinition =
        cache.getJSONDocument("serverdata/players/", session.id);
    auto const &playerBodyDefinition =
        cache.getJSONDocument("res/objects/", playerDefinition["body"]);

    json playerDef = json::object();

    playerDef.insert(playerDefinition.begin(), playerDefinition.end());
    playerDef.insert(playerBodyDefinition.begin(), playerBodyDefinition.end());

    auto &sceneManager = *sceneManagers.find(playerDef["map"])->second;
    session.sceneManager = &sceneManager;

    // TODO functions for common events
    json creationEvent;
    creationEvent["type"] = "create";
    creationEvent["sceneId"] = "player_" + session.id;
    creationEvent["def"] = playerDef;
    sceneManager.scene.submitEvents(json::array({creationEvent}));

    playerSessionsByName.insert(std::make_pair(session.id, &session));

    session.sceneManager->playerSessions.insert(
        std::make_pair(session.hdl, &session));
    sceneManager.playerSessionsByName.insert(
        std::make_pair(session.id, &session));
  } else if (j["command"] == "events") {
    session.sceneManager->scene.submitEvents(j["events"]);

    // TODO timestamp events

    for (auto peerSession_it : session.sceneManager->playerSessions) {
      auto &peerSession = *peerSession_it.second;

      if (session.id == peerSession.id || peerSession.status != "initialized")
        continue;

      m_server.send(peerSession.hdl, j.dump(),
                    websocketpp::frame::opcode::text);
    }
  } else if (j["command"] == "initialized") {
    session.status = "initialized";
  }
}

void GameServer::playerCreated(std::string id) {
  auto &session = *playerSessionsByName.find(id)->second;

  json sceneInit = session.sceneManager->wholeScene();
  sceneInit["command"] = "init scene";
  m_server.send(session.hdl, sceneInit.dump(),
                websocketpp::frame::opcode::text);
}

void GameServer::calculateDeltas() {
  for (auto sceneManager_it : sceneManagers) {
    auto &sceneManager = *sceneManager_it.second;

    // TODO callbacks to automatically add and remove SM's from active list

    if (!sceneManager.scene.running)
      continue;

    sceneManager.delta = sceneManager.sceneDelta();
  }
}

// TODO combine into makeUpdate w/ lambda
void GameServer::pushDeltas() {
  for (auto sessionEntry : playerSessions) {
    if (sessionEntry.second->status != "initialized")
      continue;

    json sceneUpdate = sessionEntry.second->sceneManager->delta;

    if (sceneUpdate["objects"].size() == 0)
      continue;

    sceneUpdate["command"] = "update";

    m_server.send(sessionEntry.first, sceneUpdate.dump(),
                  websocketpp::frame::opcode::text);
  }
}

// TODO rewrite
void GameServer::pushToClients() {
  pushingwholeSceneJson = true;
  int counter = 0;

  while (pushingwholeSceneJson) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    if (counter % 10 == 0) {
      calculateDeltas();
      pushDeltas();
    }

    ++counter;
  }
}