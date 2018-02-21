#include <iostream>
#include <math.h>

#include "scene.hpp"

Scene::~Scene() {
  for (std::map<std::string, Object *>::iterator i = objects.begin();
       i != objects.end(); ++i) {
    delete i->second;
  }
}

Object *Scene::createObject(std::string id, json def, b2Vec2 pos,
                            double angle) {
  b2BodyDef bodyDef;
  bodyDef.position = pos;
  bodyDef.angle = angle;

  b2PolygonShape box;
  box.SetAsBox(def["dimensions"][0], def["dimensions"][1]);

  b2Body *body;

  if (def["type"] == "dynamic") {
    bodyDef.type = b2_dynamicBody;
    b2FixtureDef fixtureDef;
    fixtureDef.shape = &box;
    fixtureDef.density = def["density"];
    fixtureDef.friction = def["friction"];
    body = physics.addDynamicBody(&bodyDef, &fixtureDef);
  } else if (def["type"] == "static") {
    b2BodyDef groundBodyDef;
    groundBodyDef.position = pos;
    groundBodyDef.angle = angle;
    body = physics.addStaticBody(&groundBodyDef, &box);
  }

  auto object = new Box2DObject(body);
  objects.insert(std::make_pair(id, object));
  return object;
}

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::milliseconds ms;
typedef std::chrono::duration<double> dsec;

void Scene::run() {
  running = true;

  while (running) {
    mutex.lock();
    auto start = Time::now();

    for (auto objectCs : controls) {
      Box2DObject *object = (Box2DObject *)objects.find(objectCs.first)->second;
      json cs = objectCs.second;

      b2Vec2 velocity = object->body->GetLinearVelocity();
      b2Vec2 desiredVelocity(0, velocity.y);

      if (cs["moveRight"].is_boolean() && cs["moveRight"]) {
        desiredVelocity.x += 15;
      }

      if (cs["moveLeft"].is_boolean() && cs["moveLeft"]) {
        desiredVelocity.x -= 15;
      }

      if (cs["jump"].is_boolean() && cs["jump"]) {
        desiredVelocity.y += -5;
      }

      b2Vec2 impulse = desiredVelocity - velocity;
      impulse *= object->body->GetMass();

      if (object->colliding.size() > 0)
        object->body->ApplyLinearImpulse(impulse,
                                         object->body->GetWorldCenter(), true);
    }

    physics.update();

    if (cameraFollow != nullptr)
      cameraPosition = cameraFollow->position();

    auto elapsed = Time::now() - start;
    mutex.unlock();

    // std::cout << "sleeping for "
    //           << (std::chrono::milliseconds(msTimeStep) - elapsed) <<
    //           std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(msTimeStep) -
                                elapsed);
  }
}

void Scene::updateControlState(std::string id, json cs) {
  auto objectCs = controls.find(id);
  if (objectCs == controls.end()) {
    controls.insert(std::make_pair(id, cs));
  } else {
    objectCs->second = cs;
  }
}

void Scene::stickCamera(Object *object) { cameraFollow = object; }