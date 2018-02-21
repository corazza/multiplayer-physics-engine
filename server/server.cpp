#include <fstream>
#include <iostream>
#include <set>
#include <stdio.h>

#include "nlohmann/json.hpp"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "scene.hpp"

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;
using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

using json = nlohmann::json;

struct GameServer {
  GameServer() {
    m_server.init_asio();

    m_server.set_open_handler(bind(&GameServer::on_open, this, ::_1));
    m_server.set_close_handler(bind(&GameServer::on_close, this, ::_1));
    m_server.set_message_handler(
        bind(&GameServer::on_message, this, ::_1, ::_2));
  }

  void on_open(connection_hdl hdl) {
    m_connections.insert(hdl);

    std::ifstream mapStream("server_initial.json");
    std::stringstream buffer;
    buffer << mapStream.rdbuf();

    std::string msg = buffer.str();
    m_server.send(hdl, msg, websocketpp::frame::opcode::text);
  }

  void on_close(connection_hdl hdl) { m_connections.erase(hdl); }

  void on_message(connection_hdl hdl, server::message_ptr msg) {
    // for (auto it : m_connections) {
    //   m_server.send(it, msg);
    // }
  }

  void run(uint16_t port) {
    m_server.listen(port);
    m_server.start_accept();
    m_server.run();
  }

private:
  typedef std::set<connection_hdl, std::owner_less<connection_hdl>> con_list;

  server m_server;
  con_list m_connections;
};

struct ServerSceneManager {
  Scene scene;
  std::thread updateThread;

  void loadMap(std::string name) {
    std::ifstream mapStream("res/maps/" + name + "json");
    std::stringstream buffer;
    buffer << mapStream.rdbuf();

    auto j = json::parse(buffer.str());
    int lastId = 0;

    for (auto &object : j["objects"]) {
      std::string objectResId = object["resId"];

      std::ifstream mapStream("res/objects/" + objectResId + ".json");
      std::stringstream buffer;
      buffer << mapStream.rdbuf();

      auto j2 = json::parse(buffer.str());

      auto sceneObject = scene.createObject(
          "server_load_" + lastId++, j2["box2d"],
          b2Vec2(object["position"][0], object["position"][1]),
          object["angle"]);
    }
  }

  void run() { updateThread = std::thread(&Scene::run, &scene); }

  void quit() {
    scene.running = false;
    updateThread.join();
  }
};

int main() {
  GameServer server;

  ServerSceneManager sceneManager;

  sceneManager.run();

  server.run(9002);
}