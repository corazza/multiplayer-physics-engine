#include "server.hpp"

GameServer::GameServer(int port) {
  m_server.init_asio();

  m_server.set_open_handler(bind(&GameServer::on_open, this, ::_1));
  m_server.set_close_handler(bind(&GameServer::on_close, this, ::_1));
  m_server.set_message_handler(bind(&GameServer::on_message, this, ::_1, ::_2));

  m_server.set_reuse_addr(true);

  sceneManagers.push_back(new ServerSceneManager(&cache, "first"));
  sceneManagers[0]->scene.callback = [=](Object *object) {
    if (sceneManagers[0]->scene.isPlayer(object)) {
      std::string a = "player_";
      std::string id =
          object->sceneId.substr(a.length(), object->sceneId.length());
      playerCreated(id);
    }
  };

  pushThread = std::thread(&GameServer::pushToClients, this);

  m_server.listen(port);
  m_server.start_accept();
  m_server.run();
}

GameServer::~GameServer() {
  for (std::vector<ServerSceneManager *>::iterator it = sceneManagers.begin();
       it != sceneManagers.end(); ++it) {
    delete *it;
  }

  pushingwholeSceneJson = false;
  pushThread.join();
  m_server.stop_listening();
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

    auto const &playerDefinition =
        cache.getJSONDocument("serverdata/players/", "id_" + session.id);
    auto const &playerBodyDefinition =
        cache.getJSONDocument("res/objects/", playerDefinition["body"]);

    json playerDef = json::object();

    playerDef.insert(playerDefinition.begin(), playerDefinition.end());
    playerDef.insert(playerBodyDefinition.begin(), playerBodyDefinition.end());

    // TODO functions for common events
    json creationEvent;
    creationEvent["type"] = "create";
    creationEvent["sceneId"] = "player_" + session.id;
    creationEvent["def"] = playerDef;

    sceneManagers[0]->scene.submitEvents(json::array({creationEvent}));
    session.status = "not initialized";
    session.sceneManager = sceneManagers[0];
    session.sceneManager->playerSessions.insert(std::make_pair(hdl, &session));

    playerSessionsByName.insert(std::make_pair(session.id, &session));
    session.sceneManager->playerSessionsByName.insert(
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
  for (auto sceneManager_pt : sceneManagers) {
    auto &sceneManager = *sceneManager_pt;

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