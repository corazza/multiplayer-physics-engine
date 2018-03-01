#ifndef GAME_SCENE
#define GAME_SCENE

#include <chrono>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "nlohmann/json.hpp"
#include <Box2D/Box2D.h>
#include <SDL2/SDL.h>

#include "cache.hpp"
#include "controls.hpp"
#include "physics.hpp"
#include "scene_objects.hpp"

using json = nlohmann::json;

const int msTimeStep = 15;

typedef std::chrono::high_resolution_clock Time;

struct Scene {
  std::map<std::string, Object *> objects;
  std::map<Object *, std::chrono::time_point<Time, std::chrono::nanoseconds>>
      lastAbility;
  // std::map<Box2DObject *, json> playerEvents;

  JSONCache *cache;
  bool running = false;
  Physics physics;
  b2Vec2 cameraPosition;
  Object *cameraFollow;
  std::function<void(Object *)> callback;

  Scene(JSONCache *cache)
      : cache(cache), physics(msTimeStep), cameraPosition(0, 0) {}
  ~Scene();

  // TODO JSON scene properties
  Object *createObject(std::string id, json &def);
  void removeObject(std::string sceneId);
  void run();
  void submitEvents(json events);
  void stickCamera(Object *object);
  void updateCameraPosition();

private:
  // TODO move createObject and removeObject here, event-based

  json events = json::array();
  std::vector<std::pair<json, std::function<void(Object *)>>> objectCreation;
  std::vector<std::string> objectRemoval;

  json processEvents(json events);
  void processControls();
  void processObjectCreation();
};

#endif