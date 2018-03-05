#include <fstream>
#include <iostream>
#include <stdio.h>

#include "nlohmann/json.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "client.hpp"
#include "client_network.hpp"
#include "render.hpp"

int main(int argc, char *argv[]) {
  std::string playerName;

  if (argc < 2) {
    playerName = "thalinon";
    // std::cout << "started without player name " << std::endl;
    // return 1;
  } else {
    playerName = argv[1];
  }

  SDL_Init(SDL_INIT_VIDEO);

  int fullscreenType = 0; // SDL_WINDOW_FULLSCREEN_DESKTOP;

  int windowFlags = fullscreenType | SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS |
                    SDL_WINDOW_ALLOW_HIGHDPI;

  int rendererFlags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;

  SDL_Window *window =
      SDL_CreateWindow("Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                       1000, 1000, windowFlags);

  SDL_Renderer *sdlRenderer = SDL_CreateRenderer(window, -1, rendererFlags);
  SDL_RenderSetLogicalSize(sdlRenderer, 1000, 1000);

  IMG_Init(IMG_INIT_PNG);

  //   std::string playerName = argv[1];

  std::ifstream confStream("clientdata/conf.json");
  std::stringstream buffer;
  buffer << confStream.rdbuf();
  auto conf = json::parse(buffer.str());

  Renderer renderer(sdlRenderer);
  GameClient gameClient(playerName, &renderer);
  ClientNetwork clientNetwork(&gameClient, conf);

  std::string playerSceneId = "player_" + playerName;

  SDL_Event event;
  bool running = true;

  json keyPresses;

  keyPresses["a"] = false;
  keyPresses["w"] = false;
  keyPresses["d"] = false;
  keyPresses["s"] = false;

  while (running) {
    json events = json::array();

    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        running = false;
        break;

      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_a:
          if (!keyPresses["a"]) {
            events.push_back({{"caller", playerSceneId},
                              {"type", "control"},
                              {"action", "move"},
                              {"value", "left"},
                              {"status", "start"}});
            keyPresses["a"] = true;
          }
          break;

        case SDLK_w:
          if (!keyPresses["w"]) {
            events.push_back({{"caller", playerSceneId},
                              {"type", "control"},
                              {"action", "jump"},
                              {"status", "start"}});
            keyPresses["w"] = true;
          }
          break;

        case SDLK_d:
          if (!keyPresses["d"]) {
            events.push_back({{"caller", playerSceneId},
                              {"type", "control"},
                              {"action", "move"},
                              {"value", "right"},
                              {"status", "start"}});
            keyPresses["d"] = true;
          }
          break;

        case SDLK_s:
          if (!keyPresses["s"]) {
            events.push_back({{"caller", playerSceneId},
                              {"type", "control"},
                              {"action", "crouch"},
                              {"status", "start"}});
            keyPresses["s"] = true;
          }
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
          events.push_back({{"caller", playerSceneId},
                            {"type", "control"},
                            {"action", "move"},
                            {"value", "left"},
                            {"status", "stop"}});
          keyPresses["a"] = false;
          break;

        case SDLK_w:
          events.push_back({{"caller", playerSceneId},
                            {"type", "control"},
                            {"action", "jump"},
                            {"status", "stop"}});
          keyPresses["w"] = false;
          break;

        case SDLK_d:
          events.push_back({{"caller", playerSceneId},
                            {"type", "control"},
                            {"action", "move"},
                            {"value", "right"},
                            {"status", "stop"}});
          keyPresses["d"] = false;
          break;

        case SDLK_s:
          events.push_back({{"caller", playerSceneId},
                            {"type", "control"},
                            {"action", "crouch"},
                            {"status", "stop"}});
          keyPresses["s"] = false;
          break;

        default:
          break;
        }
        break;

      case SDL_MOUSEBUTTONDOWN: {
        b2Vec2 pos = renderer.screenToScene(event.button.x, event.button.y);
        events.push_back({{"caller", playerSceneId},
                          {"type", "control"},
                          {"action", "spawn"},
                          {"status", "start"},
                          {"position", {pos.x, pos.y}}});
        break;
      };

      case SDL_MOUSEBUTTONUP: {
        b2Vec2 pos = renderer.screenToScene(event.button.x, event.button.y);
        events.push_back({{"caller", playerSceneId},
                          {"type", "control"},
                          {"action", "spawn"},
                          {"status", "stop"},
                          {"position", {pos.x, pos.y}}});
        break;
      }

      default:
        break;
      }
    }

    if (gameClient.scene != nullptr && gameClient.scene->running) {
      if (events.size() > 0) {
        gameClient.scene->submitEvents(events);
        clientNetwork.sendEvents(events);
      }

      renderer.render();
    }
  }

  gameClient.endScene();

  SDL_DestroyWindow(window);
  SDL_DestroyRenderer(sdlRenderer);
  IMG_Quit();
  SDL_Quit();
}