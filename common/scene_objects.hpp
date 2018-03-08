#ifndef SCENE_OBJECTS
#define SCENE_OBJECTS

#include <set>
#include <string>

#include <Box2D/Box2D.h>
#include <SDL2/SDL.h>

struct Controller {
  bool movingLeft = false;
  bool movingRight = false;
  bool jumping = false;
  bool crouching = false;
  bool stopping = false;

  bool active();
};

struct FixedPosition {
  b2Vec2 pos;
  float32 angle;
};

struct Object {
  std::string resId;
  std::string sceneId;
  int spawnCount = 0;

  b2Body *body;
  FixedPosition *fixedPosition;

  std::set<Object *> colliding;
  Controller *controller = nullptr;

  b2Vec2 position();
  float32 angle();
  bool resting();
};

#endif