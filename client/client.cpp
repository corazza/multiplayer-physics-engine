#include "client.hpp"
#include <iostream>
#include <stdio.h>

void GameClient::initScene(json &from) {
  scene = new Scene(&cache);
  scene->callback = [=](Object *created) { createRenderTarget(created); };
  updateFromJSON(from);
  renderer->cameraPosition = &scene->cameraPosition; // TODO fix
  updateThread = std::thread(&Scene::run, scene);
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

// TODO object deletion
// TODO smoothing/timestamps
void GameClient::updateFromJSON(json &j) {

  std::vector<std::pair<Object *, json *>> deltaObjects;

  json creationEvents;
  for (auto &object : j["objects"]) {
    auto object_it = scene->objects.find(object["sceneId"]);

    if (object_it == scene->objects.end()) {
      auto def = cache.getJSONDocument("res/objects/", object["resId"]);
      def.insert(object.begin(), object.end());
      json creationEvent;
      creationEvent["type"] = "create";
      creationEvent["def"] = def;
      creationEvent["sceneId"] = object["sceneId"];
      creationEvents.push_back(creationEvent);
    } else {
      deltaObjects.push_back(std::make_pair(object_it->second, &object));
    }
  }
  scene->submitEvents(creationEvents);

  for (auto objectDelta : deltaObjects) {
    b2Vec2 newPosition((*objectDelta.second)["position"][0],
                       (*objectDelta.second)["position"][1]);
    b2Vec2 newVelocity((*objectDelta.second)["velocity"][0],
                       (*objectDelta.second)["velocity"][1]);
    float32 newAngle = (*objectDelta.second)["angle"];
    float32 newAngularVelocity = (*objectDelta.second)["angularVelocity"];

    b2Vec2 dP = objectDelta.first->body->GetPosition() - newPosition;

    scene->beginUpdateConflict();
    objectDelta.first->body->SetTransform(newPosition, newAngle);
    objectDelta.first->body->SetLinearVelocity(newVelocity);
    objectDelta.first->body->SetAngularVelocity(newAngularVelocity);
    scene->endUpdateConflict();
  }
}