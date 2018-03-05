#ifndef SCENE_MANAGEMENT
#define SCENE_MANAGEMENT

#include <fstream>
#include <iostream>
#include <set>
#include <stdio.h>

#include "nlohmann/json.hpp"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "cache.hpp"
#include "scene.hpp"
#include "server.hpp"

using json = nlohmann::json;
using websocketpp::connection_hdl;
using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

struct PlayerSession;

struct ServerSceneManager {
  Scene scene;
  json events = json::array();
  json delta;
  std::thread updateThread;
  std::map<connection_hdl, PlayerSession *, std::owner_less<connection_hdl>>
      playerSessions;
  std::map<std::string, PlayerSession *> playerSessionsByName;

  ServerSceneManager(JSONCache *cache, std::string name);
  ~ServerSceneManager();

  void queryObject(Object *object, json &fields);
  json wholeScene();
  json sceneDelta();
};

#endif