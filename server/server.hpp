#ifndef GAME_SERVER
#define GAME_SERVER

#include <fstream>
#include <iostream>
#include <set>
#include <stdio.h>

#include "nlohmann/json.hpp"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "cache.hpp"
#include "scene.hpp"
#include "scene_management.hpp"

typedef websocketpp::server<websocketpp::config::asio> server;

using json = nlohmann::json;
using websocketpp::connection_hdl;
using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

struct ServerSceneManager;

struct PlayerSession {
  connection_hdl hdl;
  std::string id;
  std::string status;
  ServerSceneManager *sceneManager;
};

struct GameServer {
  std::thread pushThread;
  bool pushingwholeSceneJson = false;
  JSONCache cache;

  GameServer(json conf);
  ~GameServer();

private:
  server m_server;
  std::map<std::string, ServerSceneManager *> sceneManagers;
  std::map<connection_hdl, PlayerSession *, std::owner_less<connection_hdl>>
      playerSessions;
  std::map<std::string, PlayerSession *> playerSessionsByName;
  void loadMap(std::string id);

  void on_open(connection_hdl hdl);
  void on_close(connection_hdl hdl);
  void on_message(connection_hdl hdl, server::message_ptr msg);

  void playerCreated(std::string id);
  void calculateDeltas();
  void pushDeltas();
  void pushToClients();
  void handleEvent(json event);
};

#endif