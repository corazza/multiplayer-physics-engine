#include <fstream>
#include <iostream>
#include <math.h>
#include <vector>

#include "scene.hpp"

Scene::~Scene() {
  for (std::map<std::string, Object *>::iterator i = objects.begin();
       i != objects.end(); ++i) {
    delete i->second;
  }
}

// TODO add to object removal
void Scene::removeObject(std::string sceneId) {
  auto objects_it = objects.find(sceneId);
  Object *toDelete = objects_it->second;
  physics.world.DestroyBody(toDelete->body);
  objects.erase(objects_it);
  delete toDelete;
}

Object *Scene::createObject(std::string id, json &def) {
  b2Vec2 pos(def["position"][0], def["position"][1]);
  float32 angle = def["angle"];

  b2Vec2 imp(0, 0);

  if (def.find("impulse") != def.end()) {
    imp = b2Vec2(def["impulse"][0], def["impulse"][1]);
  }

  b2BodyDef bodyDef;
  bodyDef.position = pos;
  bodyDef.angle = angle;

  b2PolygonShape box;
  box.SetAsBox(def["box2d"]["dimensions"][0], def["box2d"]["dimensions"][1]);

  b2Body *body;

  if (def["box2d"]["type"] == "dynamic") {
    bodyDef.type = b2_dynamicBody;
    bodyDef.allowSleep = true;
    b2FixtureDef fixtureDef;
    fixtureDef.shape = &box;
    fixtureDef.density = def["box2d"]["density"];
    fixtureDef.friction = def["box2d"]["friction"];
    body = physics.addDynamicBody(&bodyDef, &fixtureDef);
  } else if (def["box2d"]["type"] == "static") {
    b2BodyDef groundBodyDef;
    groundBodyDef.position = pos;
    groundBodyDef.angle = angle;
    body = physics.addStaticBody(&groundBodyDef, &box);
  }

  body->ApplyLinearImpulse(imp, body->GetWorldCenter(), true);

  auto object = new Object(body);
  object->resId = def["resId"];
  object->sceneId = id;
  objects.insert(std::make_pair(id, object));

  if (def.find("controlled") != def.end() && def["controlled"])
    object->controller = new Controller;

  if (callback)
    callback(object);

  return object;
}

void Scene::processControls() {
  for (auto &objectEntry : objects) {
    auto object = objectEntry.second;

    if (object->controller == nullptr)
      continue;

    if (!object->controller->active())
      continue;

    b2Vec2 velocity = object->body->GetLinearVelocity();
    b2Vec2 desiredVelocity(0, 0);

    if (object->controller->movingLeft)
      desiredVelocity.x -= 15;
    if (object->controller->movingRight)
      desiredVelocity.x += 15;
    if (object->controller->jumping)
      desiredVelocity.y -= 15;

    b2Vec2 impulse = desiredVelocity - velocity;
    impulse *= object->body->GetMass();

    if (object->colliding.size() > 0)
      object->body->ApplyLinearImpulse(impulse, object->body->GetWorldCenter(),
                                       true);

    object->controller->stopping = false;
  }
}

json Scene::processEvents(json events) {
  json nextBatch = json::array();

  for (auto &event : events) {
    if (event["type"] == "control") {
      std::string id = event["caller"];

      auto findObject = objects.find(id);

      if (findObject == objects.end() ||
          findObject->second->controller == nullptr)
        continue;

      auto object = findObject->second;

      if (event["action"] == "move") {
        if (event["value"] == "right") {
          object->controller->movingRight = event["status"] == "start";
        } else if (event["value"] == "left") {
          object->controller->movingLeft = event["status"] == "start";
        }

        object->controller->stopping = event["status"] == "stop";
      } else if (event["action"] == "jump") {
        object->controller->jumping = event["status"] == "start";
      } else if (event["action"] == "spawn") {
        if (event["status"] == "stop") {
          b2Vec2 actionPosition(event["position"][0], event["position"][1]);
          b2Vec2 callerPosition = object->body->GetPosition();

          b2Vec2 dpos = actionPosition - callerPosition;
          dpos.Normalize();
          b2Vec2 vel = dpos;

          vel *= 350;
          dpos *= 5;

          actionPosition = callerPosition + dpos;

          json def = cache->getJSONDocument("res/objects/", "wooden_crate");

          def["angle"] = -atan2(dpos.x, dpos.y);
          def["position"][0] = actionPosition.x;
          def["position"][1] = actionPosition.y;
          def["impulse"][0] = vel.x;
          def["impulse"][1] = vel.y;

          json spawnEvent;
          spawnEvent["type"] = "create";
          spawnEvent["def"] = def;
          spawnEvent["sceneId"] =
              id + "_spawn_" + std::to_string(object->spawnCount++);

          nextBatch.push_back(spawnEvent);

          std::cout << "spawn: " << spawnEvent << std::endl;
        }
      }
    } else if (event["type"] == "create") {
      std::string sceneId = event["sceneId"];
      if (objects.find(sceneId) == objects.end()) {
        createObject(sceneId, event["def"]);
      }
    }
  }

  return nextBatch;
}

void Scene::beginUpdateConflict() { updateLock.lock(); }

void Scene::endUpdateConflict() { updateLock.unlock(); }

void Scene::run() {
  running = true;

  while (running) {
    updateLock.lock();

    auto start = Time::now();

    for (json nextBatch = processEvents(events); nextBatch.size() > 0;
         nextBatch = processEvents(nextBatch))
      ;
    events.clear();
    processControls();

    physics.update();

    // TODO fix camera
    updateCameraPosition();

    auto elapsed = Time::now() - start;

    updateLock.unlock();

    std::this_thread::sleep_for(std::chrono::milliseconds(msTimeStep) -
                                elapsed);
  }
}

void Scene::updateCameraPosition() {
  if (cameraFollow != nullptr) {
    cameraPosition = cameraFollow->body->GetPosition();
  }
}

void Scene::submitEvents(json toSubmit) {
  beginUpdateConflict();
  for (auto event : toSubmit) {
    events.push_back(event);
  }
  endUpdateConflict();
}

void Scene::stickCamera(Object *object) { cameraFollow = object; }