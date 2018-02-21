#ifndef GAME_SCENE
#define GAME_SCENE

#include <map>
#include <set>
#include <string>
#include <utility>
#include <chrono>
#include <mutex>

#include "nlohmann/json.hpp"
#include <Box2D/Box2D.h>
#include <SDL2/SDL.h>

#include "physics.hpp"
#include "scene_objects.hpp"

using json = nlohmann::json;

const int msTimeStep = 15;

struct Scene {
  std::map<std::string, Object *> objects;
  std::map<std::string, json> controls;
  Object *player;
  std::mutex mutex;
  bool running = false;
  Physics physics;
  b2Vec2 cameraPosition;
  Object *cameraFollow;

  Scene() : physics(msTimeStep), cameraPosition(0, 0) {}
  ~Scene();

  Object *createObject(std::string id, json def, b2Vec2 pos, double angle);
  void run();
  void updateControlState(std::string id, json cs);
  void stickCamera(Object *object);
};

#endif