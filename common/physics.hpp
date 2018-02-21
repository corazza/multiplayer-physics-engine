#ifndef GAME_PHYSICS
#define GAME_PHYSICS

#include <chrono>
#include <mutex>
#include <set>
#include <thread>

#include <Box2D/Box2D.h>

class Physics {
public:
  Physics(int msTimeStep);

  b2Vec2 gravity;
  b2World world;
  float32 timeStep;
  int32 velocityIterations = 6;
  int32 positionIterations = 2;
  std::set<b2Contact *> contacts;

  b2Body *addDynamicBody(b2BodyDef *def, b2FixtureDef *fixtureDef);
  b2Body *addStaticBody(b2BodyDef *def, b2PolygonShape *shape);
  void update();

private:
  class ContactListener : public b2ContactListener {
  public:
    Physics *parent;

    void BeginContact(b2Contact *contact);
    void EndContact(b2Contact *contact);
  };

  ContactListener contactListener;
};

#endif