#include <fstream>
#include <iostream>
#include <set>
#include <stdio.h>

#include "nlohmann/json.hpp"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "cache.hpp"
#include "scene.hpp"

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;
using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

using json = nlohmann::json;

struct ServerSceneManager {
  Scene scene;
  json events = json::array();
  json delta;
  std::thread updateThread;

  ServerSceneManager(JSONCache *cache, std::string name) : scene(cache) {
    auto const &mapDefinition = cache->getJSONDocument("res/maps/", name);
    json creationEvents;

    int lastId = 0;
    for (auto &object : mapDefinition["objects"]) {
      json objectDefinition =
          cache->getJSONDocument("res/objects/", object["resId"]);
      objectDefinition.insert(object.begin(), object.end());

      json creationEvent;
      creationEvent["type"] = "create";
      creationEvent["sceneId"] = "server_load_" + std::to_string(lastId++);
      creationEvent["def"] = objectDefinition;
      creationEvents.push_back(creationEvent);
    }

    scene.submitEvents(creationEvents);
    updateThread = std::thread(&Scene::run, &scene);
  }

  ~ServerSceneManager() {
    scene.running = false;
    updateThread.join();
  }

  void queryObject(Object *object, json &fields) {
    if (fields.find("position") != fields.end()) {
      b2Vec2 pos = object->body->GetPosition();
      fields["position"] = {pos.x, pos.y};
    }

    if (fields.find("angle") != fields.end()) {
      fields["angle"] = object->body->GetAngle();
    }

    if (fields.find("velocity") != fields.end()) {
      b2Vec2 vel = object->body->GetLinearVelocity();
      fields["velocity"] = {vel.x, vel.y};
    }

    if (fields.find("angularVelocity") != fields.end()) {
      fields["angularVelocity"] = object->body->GetAngularVelocity();
    }

    if (fields.find("resting") != fields.end()) {
      fields["resting"] = object->resting();
    }

    if (fields.find("resId") != fields.end()) {
      fields["resId"] = object->resId;
    }
  }

  json wholeScene() {
    json wholeSceneJson;

    wholeSceneJson["objects"] = json::array();

    for (auto object : scene.objects) {
      json objectJSON = {{"position", {0, 0}},
                         {"angle", 0},
                         {"velocity", {0, 0}},
                         {"angularVelocity", 0},
                         {"resId", ""}};

      queryObject(object.second, objectJSON);
      objectJSON["sceneId"] = object.first;
      wholeSceneJson["objects"].push_back(objectJSON);
    }

    return wholeSceneJson;
  }

  json sceneDelta() {
    json sceneDeltaJson;

    sceneDeltaJson["objects"] = json::array();

    for (auto object : scene.objects) {
      if (object.second->resting())
        continue;

      json objectJSON = {{"position", {0, 0}},
                         {"angle", 0},
                         {"velocity", {0, 0}},
                         {"angularVelocity", 0},
                         {"resId", ""}};
      queryObject(object.second, objectJSON);
      objectJSON["sceneId"] = object.first;
      sceneDeltaJson["objects"].push_back(objectJSON);
    }

    return sceneDeltaJson;
  }
};

struct PlayerSession {
  connection_hdl hdl;
  std::string id;
  std::string status;
  ServerSceneManager *sceneManager;
};

struct GameServer {
  std::vector<ServerSceneManager *> sceneManagers;
  std::map<connection_hdl, PlayerSession *, std::owner_less<connection_hdl>>
      playerSessions;
  std::map<connection_hdl, PlayerSession *, std::owner_less<connection_hdl>>
      toInitialize;

  std::thread pushThread;
  bool pushingwholeSceneJson = false;
  JSONCache cache;

  GameServer(int port) {
    m_server.init_asio();

    m_server.set_open_handler(bind(&GameServer::on_open, this, ::_1));
    m_server.set_close_handler(bind(&GameServer::on_close, this, ::_1));
    m_server.set_message_handler(
        bind(&GameServer::on_message, this, ::_1, ::_2));

    m_server.set_reuse_addr(true);

    sceneManagers.push_back(new ServerSceneManager(&cache, "first"));

    pushThread = std::thread(&GameServer::pushToClients, this);

    m_server.listen(port);
    m_server.start_accept();
    m_server.run();
  }

  ~GameServer() {
    for (std::vector<ServerSceneManager *>::iterator it = sceneManagers.begin();
         it != sceneManagers.end(); ++it) {
      delete *it;
    }

    pushingwholeSceneJson = false;
    pushThread.join();
    m_server.stop_listening();
  }

  void on_open(connection_hdl hdl) {
    auto session = new PlayerSession;
    session->hdl = hdl;
    session->status = "not logged in";
    playerSessions.insert(std::make_pair(hdl, session));
  }

  void on_close(connection_hdl hdl) {
    auto session_it = playerSessions.find(hdl);

    if (session_it == playerSessions.end()) {
      // TODO close connection
      return;
    }

    // TODO
    // sceneManagers[0]->scene.removeObject("player_" + playerId);

    playerSessions.erase(hdl);
    toInitialize.erase(hdl);

    json removeEvent =
        json::parse("{\"type\": \"remove\", \"sceneId\": \"player_" +
                    session_it->second->id + "\" }");

    session_it->second->sceneManager->events.push_back(removeEvent);
    session_it->second->sceneManager->scene.submitEvents(
        json::array({removeEvent}));
  }

  void on_message(connection_hdl hdl, server::message_ptr msg) {
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
      playerDef.insert(playerBodyDefinition.begin(),
                       playerBodyDefinition.end());

      // TODO functions for common events
      json creationEvent;
      creationEvent["type"] = "create";
      creationEvent["sceneId"] = "player_" + session.id;
      creationEvent["def"] = playerDef;

      sceneManagers[0]->scene.submitEvents(json::array({creationEvent}));
      session.status = "not initialized";
      session.sceneManager = sceneManagers[0];

      toInitialize.insert(std::make_pair(hdl, &session));
    } else if (j["command"] == "events") {
      sceneManagers[0]->scene.submitEvents(j["events"]);

      for (auto event : j["events"]) {
        sceneManagers[0]->events.push_back(event);
      }
    } else if (j["command"] == "initialized") {
      session.status = "initialized";
    }
  }

  void pushInitial() {
    auto session_it = toInitialize.begin();

    while (session_it != toInitialize.end()) {
      auto &session = *session_it->second;

      // TODO implement hasObject
      if (session.sceneManager->scene.objects.find("player_" + session.id) !=
          session.sceneManager->scene.objects.end()) {

        json sceneInit = session.sceneManager->wholeScene();
        sceneInit["command"] = "init scene";
        m_server.send(session.hdl, sceneInit.dump(),
                      websocketpp::frame::opcode::text);

        session_it = toInitialize.erase(session_it);
      } else {
        ++session_it;
      }
    }
  }

  void calculateDeltas() {
    for (auto sceneManager_pt : sceneManagers) {
      auto &sceneManager = *sceneManager_pt;

      // TODO callbacks to automatically add and remove SM's from active list

      if (!sceneManager.scene.running)
        continue;

      sceneManager.delta = sceneManager.sceneDelta();
    }
  }

  void clearEventBuffers() {
    for (auto sceneManager_pt : sceneManagers) {
      auto &sceneManager = *sceneManager_pt;

      sceneManager.events = json::array();
    }
  }

  // TODO combine into makeUpdate w/ lambda
  void pushDeltas() {
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

  void pushEvents() {
    for (auto sessionEntry : playerSessions) {
      if (sessionEntry.second->status != "initialized")
        continue;
      if (sessionEntry.second->sceneManager->events.size() == 0)
        continue;

      json eventsCommand;

      eventsCommand["command"] = "events";
      eventsCommand["events"] = sessionEntry.second->sceneManager->events;

      m_server.send(sessionEntry.first, eventsCommand.dump(),
                    websocketpp::frame::opcode::text);
    }
  }

  void pushToClients() {
    pushingwholeSceneJson = true;
    int counter = 0;

    while (pushingwholeSceneJson) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));

      pushInitial();
      // pushEvents();
      // clearEventBuffers();

      if (counter % 10 == 0) {
        // calculateDeltas();
        // pushDeltas();
      }

      ++counter;
    }
  }

private:
  server m_server;
};

int main() {
  GameServer server(9003);
  std::cout << "clean server exit" << std::endl;
}