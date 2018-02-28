#ifndef GAME_CLIENT_NETWORK
#define GAME_CLIENT_NETWORK

#include "nlohmann/json.hpp"
#include <Box2D/Box2D.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

#include "client.hpp"

using json = nlohmann::json;

typedef websocketpp::client<websocketpp::config::asio_client> client;

// pull out the type of messages sent by our config
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

using websocketpp::connection_hdl;
using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

struct ClientNetwork {
  client endpoint;
  std::string uri = "ws://localhost:9003";
  client::connection_ptr conn;
  connection_hdl handle;
  std::thread connThread;
  GameClient *gameClient;

  ClientNetwork(GameClient *gameClient);
  ~ClientNetwork();

  void runConnection();

  void on_open(connection_hdl hdl);
  void on_fail(connection_hdl hdl);
  void on_message(connection_hdl hdl, message_ptr msg);
  void on_close(connection_hdl hdl);

  void sendEvents(json &events);
};

#endif