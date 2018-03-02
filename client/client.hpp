#ifndef GAME_CLIENT
#define GAME_CLIENT

#include "nlohmann/json.hpp"
#include <Box2D/Box2D.h>
#include <SDL2/SDL.h>
//#include <SDL2/SDL_image.h>

#include "scene.hpp"
#include "scene_objects.hpp"
#include "cache.hpp"
#include "render.hpp"

using json = nlohmann::json;

struct GameClient {
  JSONCache cache;
  std::string playerId;
  Scene *scene = nullptr;
  std::thread updateThread;
  Renderer *renderer;

  GameClient(std::string playerId, Renderer *renderer)
      : playerId(playerId), renderer(renderer) {}

  json creationEvent(json &object);
  void initScene(json &from);
  void updateScene(json &from);
  void endScene();
  void createRenderTarget(Object *object);
  void removeRenderTarget(Object *object);
};

#endif