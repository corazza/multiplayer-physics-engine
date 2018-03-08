#include <algorithm>
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

void Scene::removeObject(std::string sceneId) {
  auto objects_it = objects.find(sceneId);
  Object *toDelete = objects_it->second;

  if (toDelete->body != nullptr)
    physics.world.DestroyBody(toDelete->body);

  objects.erase(objects_it);

  if (removedCallback) {
    removedCallback(toDelete);
  }

  delete toDelete;
}

Object *Scene::createObject(std::string id, json &def) {
  auto object = new Object;
  object->resId = def["resId"];
  object->sceneId = id;

  b2Vec2 pos(def["position"][0], def["position"][1]);
  float32 angle = def["angle"];

  if (def["type"] == "box2d") {
    b2Vec2 imp(0, 0);

    if (def["box2d"].find("impulse") != def["box2d"].end()) {
      imp = b2Vec2(def["box2d"]["impulse"][0], def["box2d"]["impulse"][1]);
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
    body->SetUserData(object);
    object->body = body;
  } else if (def["type"] == "background") {
    object->fixedPosition = new FixedPosition;
    object->fixedPosition->pos = pos;
    object->fixedPosition->angle = angle;
  }

  if (std::find(def["flags"].begin(), def["flags"].end(), "controlled") !=
      def["flags"].end()) {
    object->controller = new Controller;
    object->body->SetFixedRotation(true);
  }

  objects.insert(std::make_pair(id, object));

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
      desiredVelocity.y -= 10;

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
          b2Vec2 callerPosition = object->position();

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
          def["box2d"]["impulse"][0] = vel.x;
          def["box2d"]["impulse"][1] = vel.y;

          json spawnEvent;
          spawnEvent["type"] = "create";
          spawnEvent["def"] = def;
          spawnEvent["sceneId"] =
              "spawn_" + id + "_" + std::to_string(spawnCounter++);

          nextBatch.push_back(spawnEvent);
        }
      } else if (event["action"] == "use") {
        std::cout << "use by " << id << std::endl;
      }
    } else if (event["type"] == "create") {
      std::string sceneId = event["sceneId"];
      if (objects.find(sceneId) == objects.end())
        createObject(sceneId, event["def"]);
    } else if (event["type"] == "modify") {
      std::string id = event["sceneId"];

      auto findObject = objects.find(id);

      b2Vec2 newPosition(event["newPosition"][0], event["newPosition"][1]);
      b2Vec2 newVelocity(event["newVelocity"][0], event["newVelocity"][1]);
      float32 newAngle = event["newAngle"];
      float32 newAngularVelocity = event["newAngularVelocity"];

      findObject->second->body->SetTransform(newPosition, newAngle);
      findObject->second->body->SetLinearVelocity(newVelocity);
      findObject->second->body->SetAngularVelocity(newAngularVelocity);
    } else if (event["type"] == "remove") {
      removeObject(event["sceneId"]);
    }
  }

  return nextBatch;
}

void Scene::run() {
  running = true;

  while (running) {
    auto start = Time::now();

    events = processEvents(events);

    processControls();

    physics.update();

    // TODO fix camera
    updateCameraPosition();

    auto elapsed = Time::now() - start;

    std::this_thread::sleep_for(std::chrono::milliseconds(msTimeStep) -
                                elapsed);
  }
}

void Scene::updateCameraPosition() {
  if (cameraFollow != nullptr) {
    cameraPosition = cameraFollow->position();
  }
}

void Scene::submitEvents(json toSubmit) {
  for (auto event : toSubmit) {
    events.push_back(event);
  }
}

void Scene::stickCamera(Object *object) { cameraFollow = object; }

bool Scene::isPlayer(Object *object) {
  std::string a = "player_";
  return object->sceneId.compare(0, a.length(), a) == 0;
}