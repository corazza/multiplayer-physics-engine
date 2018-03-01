#include "client.hpp"
#include <iostream>
#include <stdio.h>

json GameClient::creationEvent(json &object) {
  auto def = cache.getJSONDocument("res/objects/", object["resId"]);
  def.insert(object.begin(), object.end());
  json creationEvent;
  creationEvent["type"] = "create";
  creationEvent["def"] = def;
  creationEvent["sceneId"] = object["sceneId"];
  return creationEvent;
}

void GameClient::initScene(json &from) {
  std::cout << "initializing scene" << std::endl;
  scene = new Scene(&cache);
  scene->callback = [=](Object *created) { createRenderTarget(created); };

  json creationEvents = json::array();
  for (auto &object : from["objects"]) {
    creationEvents.push_back(creationEvent(object));
  }
  scene->submitEvents(creationEvents);

  renderer->cameraPosition = &scene->cameraPosition; // TODO fix
  updateThread = std::thread(&Scene::run, scene);
}

// TODO object deletion
// TODO smoothing/timestamps
void GameClient::updateScene(json &from) {
  json events = json::array();

  for (auto &object : from["objects"]) {
    auto object_it = scene->objects.find(object["sceneId"]);

    if (object_it == scene->objects.end()) {
      events.push_back(creationEvent(object));
    } else {
      json modEvent;
      modEvent["type"] = "modify";
      modEvent["sceneId"] = object["sceneId"];
      modEvent["newPosition"] = object["position"];
      modEvent["newVelocity"] = object["velocity"];
      modEvent["newAngle"] = object["angle"];
      modEvent["newAngularVelocity"] = object["angularVelocity"];
      events.push_back(modEvent);
    }
  }

  scene->submitEvents(events);
}

void GameClient::endScene() {
  renderer->removeAll();

  if (scene != nullptr) {
    scene->running = false;
    updateThread.join();
    delete scene;
  }
}

void GameClient::createRenderTarget(Object *object) {
  auto const &resourceJSON =
      cache.getJSONDocument("res/objects/", object->resId);

  b2Vec2 dim(resourceJSON["box2d"]["dimensions"][0],
             resourceJSON["box2d"]["dimensions"][1]);

  if (resourceJSON["render"]["type"] == "rect") {
    auto texture = renderer->getTexture(resourceJSON["render"]["image"]);
    renderer->add(new RectTarget(object, dim, texture));
  } else if (resourceJSON["render"]["type"] == "tiled") {
    b2Vec2 tileDim(resourceJSON["render"]["tileDim"][0],
                   resourceJSON["render"]["tileDim"][1]);
    auto texture = renderer->getTexture(resourceJSON["render"]["tiledWith"]);
    renderer->add(new TileTarget(object, dim, texture, tileDim));
  }

  if (object->sceneId == "player_" + playerId) {
    scene->stickCamera(object);
  }
}