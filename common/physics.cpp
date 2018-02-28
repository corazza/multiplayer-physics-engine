#include <iostream>

#include "physics.hpp"
#include "scene_objects.hpp"

Physics::Physics(int msTimeStep)
    : world(b2Vec2(0, 10)), timeStep(msTimeStep / 1000.0) {
  contactListener.parent = this;
  world.SetContactListener(&contactListener);
}

void Physics::ContactListener::BeginContact(b2Contact *contact) {
  Object *a = (Object *)contact->GetFixtureA()->GetBody()->GetUserData();

  Object *b = (Object *)contact->GetFixtureB()->GetBody()->GetUserData();

  a->colliding.insert(b);
  b->colliding.insert(a);
}

void Physics::ContactListener::EndContact(b2Contact *contact) {
  Object *a = (Object *)contact->GetFixtureA()->GetBody()->GetUserData();

  Object *b = (Object *)contact->GetFixtureB()->GetBody()->GetUserData();

  a->colliding.erase(b);
  b->colliding.erase(a);
}

b2Body *Physics::addDynamicBody(b2BodyDef *bodyDef, b2FixtureDef *fixtureDef) {
  b2Body *body = world.CreateBody(bodyDef);
  body->CreateFixture(fixtureDef);
  return body;
}

b2Body *Physics::addStaticBody(b2BodyDef *bodyDef, b2PolygonShape *shape) {
  b2Body *body = world.CreateBody(bodyDef);
  body->CreateFixture(shape, 0.0f);
  return body;
}

void Physics::update() {
  ++deltaCounter;
  world.Step(timeStep, velocityIterations, positionIterations);
}

int Physics::deltaCounterSinceLastCall() {
  int a = deltaCounter;
  deltaCounter = 0;
  return a;
}