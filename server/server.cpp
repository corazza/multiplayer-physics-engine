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
  std::vector<json> eventsBuffer;
  std::thread updateThread;
  typedef std::set<connection_hdl, std::owner_less<connection_hdl>> con_list;

  con_list m_connections;
  con_list toSendEntireScene;

  std::map<connection_hdl, std::string, std::owner_less<connection_hdl>>
      hdlToId;

  std::map<std::string, connection_hdl> idToHdl;

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

struct GameServer {
  std::vector<ServerSceneManager *> sceneManagers;
  std::thread pushThread;
  bool pushingwholeSceneJson = false;
  JSONCache cache;

  GameServer() {
    m_server.init_asio();

    m_server.set_open_handler(bind(&GameServer::on_open, this, ::_1));
    m_server.set_close_handler(bind(&GameServer::on_close, this, ::_1));
    m_server.set_message_handler(
        bind(&GameServer::on_message, this, ::_1, ::_2));

    m_server.set_reuse_addr(true);
  }

  ~GameServer() {
    for (std::vector<ServerSceneManager *>::iterator it = sceneManagers.begin();
         it != sceneManagers.end(); ++it) {
      delete *it;
    }
  }

  std::string getPlayerIdByHdl(connection_hdl hdl) {
    auto it = sceneManagers[0]->hdlToId.find(hdl);

    if (it == sceneManagers[0]->hdlToId.end())
      throw std::invalid_argument("invalid player connection");

    return it->second;
  }

  connection_hdl getHdlByPlayerId(std::string id) {
    auto it = sceneManagers[0]->idToHdl.find(id);

    if (it == sceneManagers[0]->idToHdl.end())
      throw std::invalid_argument("invalid player id: " + id);

    return it->second;
  }

  void on_open(connection_hdl hdl) {
    sceneManagers[0]->m_connections.insert(hdl);
  }

  void on_close(connection_hdl hdl) {
    std::string playerId = getPlayerIdByHdl(hdl);

    std::cout << "player with id " << playerId << " logged out!" << std::endl;

    sceneManagers[0]->scene.removeObject("player_" + playerId);

    sceneManagers[0]->m_connections.erase(hdl);
    sceneManagers[0]->hdlToId.erase(hdl);
    sceneManagers[0]->idToHdl.erase(playerId);

    // if (sceneManagers[0]->m_connections.size() == 0) {
    //   quit();
    // }
  }

  void on_message(connection_hdl hdl, server::message_ptr msg) {
    auto j = json::parse(msg->get_payload());

    if (j["command"] == "login") {
      std::string name = j["id"]; // TODO consistent id/name
      sceneManagers[0]->hdlToId.insert(std::make_pair(hdl, name));
      sceneManagers[0]->idToHdl.insert(std::make_pair(name, hdl));

      auto const &playerDefinition =
          cache.getJSONDocument("serverdata/players/", "id_" + name);
      auto const &playerBodyDefinition =
          cache.getJSONDocument("res/objects/", playerDefinition["body"]);

      json playerDef = json::object();

      playerDef.insert(playerDefinition.begin(), playerDefinition.end());
      playerDef.insert(playerBodyDefinition.begin(),
                       playerBodyDefinition.end());

      // TODO functions for common events
      json creationEvent;
      creationEvent["type"] = "create";
      creationEvent["sceneId"] = "player_" + name;
      creationEvent["def"] = playerDef;

      sceneManagers[0]->scene.submitEvents(json::array({creationEvent}));

      sceneManagers[0]->toSendEntireScene.insert(hdl);
    } else if (j["command"] == "events") {
      std::string playerId = getPlayerIdByHdl(hdl);

      sceneManagers[0]->scene.submitEvents(j["events"]);

      for (auto event : j["events"]) {
        sceneManagers[0]->eventsBuffer.push_back(event);
      }
    }
  }

  void run(uint16_t port) {
    sceneManagers.push_back(new ServerSceneManager(&cache, "first"));

    pushThread = std::thread(&GameServer::pushToClients, this);

    m_server.listen(port);
    m_server.start_accept();
    m_server.run();
  }

  void quit() {
    pushingwholeSceneJson = false;
    pushThread.join();
    m_server.stop_listening();
  }

  void pushInitial() {
    for (auto sceneManager : sceneManagers) {
      if (!sceneManager->scene.running)
        continue;

      json sceneInit = sceneManager->wholeScene();
      sceneInit["command"] = "init scene";

      auto hdl_it = sceneManager->toSendEntireScene.begin();

      while (hdl_it != sceneManager->toSendEntireScene.end()) {
        auto hdl = *hdl_it;
        std::string playerId = getPlayerIdByHdl(hdl);
        // TODO implement hasObject
        if (sceneManager->scene.objects.find("player_" + playerId) !=
            sceneManager->scene.objects.end()) {
          m_server.send(hdl, sceneInit.dump(),
                        websocketpp::frame::opcode::text);
          hdl_it = sceneManager->toSendEntireScene.erase(hdl_it);
        } else {
          ++hdl_it;
        }
      }
    }
  }

  // TODO combine into makeUpdate w/ lambda
  void pushDeltas() {
    for (std::vector<ServerSceneManager *>::iterator it = sceneManagers.begin();
         it != sceneManagers.end(); ++it) {

      ServerSceneManager *sceneManager = *it;

      if (!sceneManager->scene.running)
        continue;

      json sceneUpdate = sceneManager->sceneDelta();

      if (sceneUpdate["objects"].size() == 0)
        continue;

      sceneUpdate["command"] = "update";

      for (auto it : sceneManager->m_connections) {
        m_server.send(it, sceneUpdate.dump(), websocketpp::frame::opcode::text);
      }
    }
  }

  void pushEvents() {
    for (std::vector<ServerSceneManager *>::iterator it = sceneManagers.begin();
         it != sceneManagers.end(); ++it) {

      ServerSceneManager *sceneManager = *it;

      if (!sceneManager->scene.running)
        continue;

      if (sceneManager->eventsBuffer.empty())
        continue;

      json eventsCommand;

      eventsCommand["command"] = "events";
      eventsCommand["events"] = sceneManager->eventsBuffer;

      sceneManager->eventsBuffer.clear();

      std::cout << "sending event update command: " << eventsCommand
                << std::endl;

      for (auto it : sceneManager->m_connections) {
        m_server.send(it, eventsCommand.dump(),
                      websocketpp::frame::opcode::text);
      }
    }
  }

  void pushToClients() {
    pushingwholeSceneJson = true;
    int counter = 0;

    while (pushingwholeSceneJson) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));

      if (counter % 10 == 0) {
        pushDeltas();
        pushInitial();
      }

      pushEvents();

      ++counter;
    }
  }

private:
  server m_server;
};

int main() {
  GameServer server;
  server.run(9003);

  std::cout << "clean server exit" << std::endl;
}