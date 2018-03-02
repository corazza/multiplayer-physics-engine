#include "client_network.hpp"

ClientNetwork::ClientNetwork(GameClient *gameClient) : gameClient(gameClient) {
  endpoint.clear_access_channels(websocketpp::log::alevel::frame_header);
  endpoint.clear_access_channels(websocketpp::log::alevel::frame_payload);
  // endpoint.set_error_channels(websocketpp::log::elevel::none);

  endpoint.init_asio();
  endpoint.start_perpetual();

  endpoint.set_open_handler(bind(&ClientNetwork::on_open, this, ::_1));
  endpoint.set_fail_handler(bind(&ClientNetwork::on_fail, this, ::_1));
  endpoint.set_message_handler(
      bind(&ClientNetwork::on_message, this, ::_1, ::_2));
  endpoint.set_close_handler(bind(&ClientNetwork::on_close, this, ::_1));

  websocketpp::lib::error_code ec;
  conn = endpoint.get_connection(uri, ec);
  endpoint.connect(conn);

  connThread = std::thread(&ClientNetwork::runConnection, this);
}

ClientNetwork::~ClientNetwork() {
  endpoint.stop_perpetual();

  websocketpp::lib::error_code ec;
  endpoint.close(conn->get_handle(), websocketpp::close::status::going_away, "",
                 ec);

  connThread.join();
}

void ClientNetwork::runConnection() { endpoint.run(); }

void ClientNetwork::on_open(connection_hdl hdl) {
  endpoint.get_alog().write(websocketpp::log::alevel::app,
                            "Connected to server");

  json loginCommand;
  loginCommand["command"] = "login";
  loginCommand["id"] = gameClient->playerId;

  handle = hdl;
  endpoint.send(hdl, loginCommand.dump(), websocketpp::frame::opcode::text);
}

void ClientNetwork::on_fail(connection_hdl hdl) {
  endpoint.get_alog().write(websocketpp::log::alevel::app, "Connection Failed");
  gameClient->endScene();
}

void ClientNetwork::on_message(connection_hdl hdl, message_ptr msg) {
  endpoint.get_alog().write(websocketpp::log::alevel::app,
                            "Received Reply: " + msg->get_payload());

  auto j = json::parse(msg->get_payload());

  if (j["command"] == "init scene") {
    gameClient->initScene(j);
    endpoint.send(hdl,
                  "{ \"command\": \"initialized\" }"_json.dump(), // TODO better
                  websocketpp::frame::opcode::text);
    endpoint.get_alog().write(websocketpp::log::alevel::app,
                              "Sent initialized command");
  } else if (j["command"] == "end scene") {
    gameClient->endScene();
  } else if (j["command"] == "update") {
    if (gameClient->scene->running)
      gameClient->updateScene(j);
  } else if (j["command"] == "events") {
    json filteredEvents = json::array();

    for (auto &event : j["events"]) {
      if (event["caller"] != "player_" + gameClient->playerId) {
        filteredEvents.push_back(event);
      }
    }

    gameClient->scene->submitEvents(filteredEvents);
  }
}

void ClientNetwork::on_close(connection_hdl hdl) {
  endpoint.get_alog().write(websocketpp::log::alevel::app, "Connection Closed");
}

void ClientNetwork::sendEvents(json &events) {
  json eventsCommand;

  eventsCommand["command"] = "events";
  eventsCommand["events"] = events;

  endpoint.send(handle, eventsCommand.dump(), websocketpp::frame::opcode::text);
}