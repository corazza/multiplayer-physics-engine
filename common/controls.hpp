#ifndef GAME_CONTROLS
#define GAME_CONTROLS

#include <string>

#include "nlohmann/json.hpp"

#include "scene_objects.hpp"

using json = nlohmann::json;

struct ControlObject {
  Object *controlling;
  json current;
  json previous;
};

#endif