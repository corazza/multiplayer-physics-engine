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
  JSONCache *cache;
  bool running = false;
  Physics physics;

  std::map<std::string, Object *> objects;

  b2Vec2 cameraPosition;
  Object *cameraFollow;
  std::function<void(Object *)> callback;
  std::function<void(Object *)> removedCallback;
  std::function<void(json)> externalEventHandler;
  int spawnCounter = 0;

  Scene(JSONCache *cache)
      : cache(cache), physics(msTimeStep), cameraPosition(0, 0) {}
  ~Scene();

  void run();
  void submitEvents(json events);
  void stickCamera(Object *object);
  void updateCameraPosition();
  bool isPlayer(Object *object);

private:
  // TODO move createObject and removeObject here, event-based

  json events = json::array();
  std::vector<std::pair<json, std::function<void(Object *)>>> objectCreation;
  std::vector<std::string> objectRemoval;
  std::map<std::string, Object *> uses;

  Object *createObject(std::string id, json &def);
  void removeObject(std::string sceneId);

  json processEvents(json events);
  void processControls();
  void processObjectCreation();
};

#endif