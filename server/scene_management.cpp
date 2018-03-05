#include "scene_management.hpp"

ServerSceneManager::ServerSceneManager(JSONCache *cache, std::string name)
    : scene(cache) {
  auto const &mapDefinition = cache->getJSONDocument("res/maps/", name);
  json creationEvents;

  int lastId = 0;
  for (auto &object : mapDefinition["objects"]) {
    json objectDefinition =
        cache->getJSONDocument("res/objects/", object["resId"]);
    objectDefinition.insert(object.begin(), object.end());

    json creationEvent;
    creationEvent["type"] = "create";
    creationEvent["sceneId"] = "server_load_" + std::to_string(lastId++);
    creationEvent["def"] = objectDefinition;
    creationEvents.push_back(creationEvent);
  }

  scene.submitEvents(creationEvents);
  updateThread = std::thread(&Scene::run, &scene);
}

ServerSceneManager::~ServerSceneManager() {
  scene.running = false;
  updateThread.join();
}

void ServerSceneManager::queryObject(Object *object, json &fields) {
  if (fields.find("position") != fields.end()) {
    b2Vec2 pos = object->body->GetPosition();
    fields["position"] = {pos.x, pos.y};
  }

  if (fields.find("angle") != fields.end()) {
    fields["angle"] = object->body->GetAngle();
  }

  if (fields.find("velocity") != fields.end()) {
    b2Vec2 vel = object->body->GetLinearVelocity();
    fields["velocity"] = {vel.x, vel.y};
  }

  if (fields.find("angularVelocity") != fields.end()) {
    fields["angularVelocity"] = object->body->GetAngularVelocity();
  }

  if (fields.find("resting") != fields.end()) {
    fields["resting"] = object->resting();
  }

  if (fields.find("resId") != fields.end()) {
    fields["resId"] = object->resId;
  }
}

json ServerSceneManager::wholeScene() {
  json wholeSceneJson;

  wholeSceneJson["objects"] = json::array();
  wholeSceneJson["spawn count"] = scene.spawnCounter;

  for (auto object : scene.objects) {
    json objectJSON = {{"position", {0, 0}},
                       {"angle", 0},
                       {"velocity", {0, 0}},
                       {"angularVelocity", 0},
                       {"resId", ""}};

    queryObject(object.second, objectJSON);
    objectJSON["sceneId"] = object.first;
    wholeSceneJson["objects"].push_back(objectJSON);
  }

  return wholeSceneJson;
}

json ServerSceneManager::sceneDelta() {
  json sceneDeltaJson;

  sceneDeltaJson["objects"] = json::array();

  for (auto object : scene.objects) {
    if (object.second->resting())
      continue;

    json objectJSON = {{"position", {0, 0}},
                       {"angle", 0},
                       {"velocity", {0, 0}},
                       {"angularVelocity", 0},
                       {"resId", ""}};
    queryObject(object.second, objectJSON);
    objectJSON["sceneId"] = object.first;
    sceneDeltaJson["objects"].push_back(objectJSON);
  }

  return sceneDeltaJson;
}
