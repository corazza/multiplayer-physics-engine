#include <iostream>

#include "physics.hpp"
#include "scene_objects.hpp"

Physics::Physics(int msTimeStep)
    : gravity(0.0f, 10.0f), world(gravity), timeStep(msTimeStep / 1000.0) {
  contactListener.parent = this;
  world.SetContactListener(&contactListener);
}

void Physics::ContactListener::BeginContact(b2Contact *contact) {
  Box2DObject *a =
      (Box2DObject *)contact->GetFixtureA()->GetBody()->GetUserData();

  Box2DObject *b =
      (Box2DObject *)contact->GetFixtureB()->GetBody()->GetUserData();

  a->colliding.insert(b);
  b->colliding.insert(a);
}

void Physics::ContactListener::EndContact(b2Contact *contact) {
  Box2DObject *a =
      (Box2DObject *)contact->GetFixtureA()->GetBody()->GetUserData();

  Box2DObject *b =
      (Box2DObject *)contact->GetFixtureB()->GetBody()->GetUserData();

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
  // printf("updating with timeStep=%f\n", timeStep);
  world.Step(timeStep, velocityIterations, positionIterations);
}