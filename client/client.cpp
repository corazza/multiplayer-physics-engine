#include <fstream>
#include <iostream>
#include <stdio.h>

#include "nlohmann/json.hpp"
#include <Box2D/Box2D.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

#include "physics.hpp"
#include "render.hpp"
#include "scene.hpp"

using json = nlohmann::json;

typedef websocketpp::client<websocketpp::config::asio_client> client;

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

// pull out the type of messages sent by our config
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

using websocketpp::connection_hdl;
using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

struct GameClient {
  client endpoint;
  std::string uri = "ws://localhost:9002";
  client::connection_ptr conn;
  Scene *scene = nullptr;
  std::thread updateThread;
  std::thread connThread;
  Renderer *renderer;

  GameClient(Renderer *renderer) : renderer(renderer) {
    endpoint.clear_access_channels(websocketpp::log::alevel::frame_header);
    endpoint.clear_access_channels(websocketpp::log::alevel::frame_payload);
    // endpoint.set_error_channels(websocketpp::log::elevel::none);

    endpoint.init_asio();
    endpoint.start_perpetual();

    endpoint.set_open_handler(bind(&GameClient::on_open, this, ::_1));
    endpoint.set_fail_handler(bind(&GameClient::on_fail, this, ::_1));
    endpoint.set_message_handler(
        bind(&GameClient::on_message, this, ::_1, ::_2));
    endpoint.set_close_handler(bind(&GameClient::on_close, this, ::_1));

    websocketpp::lib::error_code ec;
    conn = endpoint.get_connection(uri, ec);
    endpoint.connect(conn);

    connThread = std::thread(&GameClient::runConnection, this);
  }

  ~GameClient() {
    endpoint.stop_perpetual();

    websocketpp::lib::error_code ec;
    endpoint.close(conn->get_handle(), websocketpp::close::status::going_away,
                   "", ec);

    connThread.join();
  }

  void on_open(connection_hdl hdl) {
    endpoint.get_alog().write(websocketpp::log::alevel::app,
                              "Connected to server");
  }

  void on_fail(connection_hdl hdl) {
    endpoint.get_alog().write(websocketpp::log::alevel::app,
                              "Connection Failed");
  }

  void on_message(connection_hdl hdl, message_ptr msg) {
    endpoint.get_alog().write(websocketpp::log::alevel::app,
                              "Received Reply: " + msg->get_payload());

    auto j = json::parse(msg->get_payload());

    if (j["command"] == "init scene") {
      scene = new Scene;
      updateFromJSON(j);
      scene->stickCamera(scene->objects.find("player")->second);
      renderer->cameraPosition = &scene->cameraPosition;
      updateThread = std::thread(&Scene::run, scene);
    } else if (j["command"] == "end scene") {
      endScene();
      delete scene;
    }
  }

  void on_close(connection_hdl hdl) {
    endpoint.get_alog().write(websocketpp::log::alevel::app,
                              "Connection Closed");
  }

  void runConnection() { endpoint.run(); }

  void endScene() {
    renderer->removeAll();
    scene->running = false;
    updateThread.join();
  }

  void updateFromJSON(json j) {
    for (auto &object : j["objects"]) {
      if (scene->objects.find(object["sceneId"]) == scene->objects.end()) {
        std::string objectResId = object["resId"];

        std::ifstream mapStream("res/objects/" + objectResId + ".json");
        std::stringstream buffer;
        buffer << mapStream.rdbuf();

        auto j2 = json::parse(buffer.str());

        auto sceneObject = scene->createObject(
            object["sceneId"], j2["box2d"],
            b2Vec2(object["position"][0], object["position"][1]),
            object["angle"]);

        b2Vec2 dim(j2["box2d"]["dimensions"][0], j2["box2d"]["dimensions"][1]);
        RenderTarget *target;

        if (j2["render"]["type"] == "rect") {
          auto texture = renderer->getTexture(j2["render"]["image"]);
          auto rectTarget = new RectTarget(sceneObject, dim, texture);
          target = rectTarget;
        } else if (j2["render"]["type"] == "tiled") {
          b2Vec2 tileDim(j2["render"]["tileDim"][0],
                         j2["render"]["tileDim"][1]);
          auto texture = renderer->getTexture(j2["render"]["tiledWith"]);
          auto tileTarget = new TileTarget(sceneObject, dim, texture, tileDim);
          target = tileTarget;
        }

        renderer->add(target);
      }
    }
  }
};

int main(int argc, char *argv[]) {
  SDL_Init(SDL_INIT_VIDEO);

  int fullscreenType = SDL_WINDOW_FULLSCREEN_DESKTOP;

  int windowFlags = fullscreenType | SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS |
                    SDL_WINDOW_ALLOW_HIGHDPI;

  int rendererFlags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;

  SDL_Window *window =
      SDL_CreateWindow("Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                       640, 480, windowFlags);

  SDL_Renderer *sdlRenderer = SDL_CreateRenderer(window, -1, rendererFlags);
  SDL_RenderSetLogicalSize(sdlRenderer, 1920, 1080);

  Renderer renderer(sdlRenderer);
  GameClient gameClient(&renderer);

  // gameClient.login("player")

  SDL_Event event;
  bool running = true;

  json cs_prev;
  json cs;

  while (running) {

    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        running = false;
        break;

      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_a:
          cs["moveLeft"] = true;
          break;

        case SDLK_w:
          cs["jump"] = true;
          break;

        case SDLK_d:
          cs["moveRight"] = true;
          break;

        case SDLK_s:
          cs["moveCrouch"] = true;
          break;

        case SDLK_ESCAPE:
          running = false;
          break;

        default:
          break;
        }
        break;

      case SDL_KEYUP:
        switch (event.key.keysym.sym) {
        case SDLK_a:
          cs["moveLeft"] = false;
          break;

        case SDLK_w:
          cs["jump"] = false;
          break;

        case SDLK_d:
          cs["moveRight"] = false;
          break;

        case SDLK_s:
          cs["crouch"] = false;
          break;

        default:
          break;
        }
        break;

        // case SDL_MOUSEBUTTONDOWN: {
        //   auto box = scene.createObject(
        //       playerDef, renderer.screenToScene(event.button.x,
        //       event.button.y), 0);
        //   auto boxTarget = new RectTarget(box, crate_img);
        //   renderer.add(boxTarget);
        //   break;
        // };

      default:
        break;
      }
    }

    if (gameClient.scene != nullptr && gameClient.scene->running) {
      gameClient.scene->updateControlState("player", cs);
      cs_prev = cs;
      renderer.render();
    }
  }

  gameClient.endScene();

  SDL_DestroyWindow(window);
  SDL_Quit();
}