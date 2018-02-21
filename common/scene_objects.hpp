#ifndef SCENE_OBJECTS
#define SCENE_OBJECTS

#include <set>

#include <Box2D/Box2D.h>
#include <SDL2/SDL.h>

struct Object {
  int id;

  virtual b2Vec2 position() = 0;
  virtual double angle() = 0;
};

struct Box2DObject : Object {
  Box2DObject(b2Body *body);

  b2Body *body;

  std::set<Box2DObject *> colliding;

  b2Vec2 position() override;
  double angle() override;
};

#endif