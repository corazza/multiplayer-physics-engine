#ifndef SCENE_OBJECTS
#define SCENE_OBJECTS

#include <set>
#include <string>

#include "nlohmann/json.hpp"
#include <Box2D/Box2D.h>
#include <SDL2/SDL.h>

using json = nlohmann::json;

struct Controller {
  bool movingLeft = false;
  bool movingRight = false;
  bool jumping = false;
  bool crouching = false;
  bool stopping = false;

  bool active();
};

struct Use {
  json eventType;
};

struct FixedPosition {
  b2Vec2 pos;
  float32 angle;
  b2Vec2 dim;
};

struct Object {
  std::string resId;
  std::string sceneId;
  int spawnCount = 0;

  b2Body *body;
  FixedPosition *fixedPosition;
  Use *use;

  std::set<Object *> colliding;
  Controller *controller = nullptr;

  b2Vec2 position();
  float32 angle();
  bool resting();
};

#endif