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

struct Object {
  std::string resId;
  std::string sceneId;
  int spawnCount = 0;

  b2Body *body;

  std::set<Object *> colliding;
  Controller *controller = nullptr;

  Object(b2Body *body);

  bool resting();
};

#endif