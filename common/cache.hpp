#ifndef GAME_CACHE
#define GAME_CACHE

#include "nlohmann/json.hpp"
#include <SDL2/SDL.h>

using json = nlohmann::json;

struct JSONCache {
  std::map<std::string, json> JSONDocuments;

  json &getJSONDocument(std::string path, std::string id);
};

#endif