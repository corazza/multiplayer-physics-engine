#include "cache.hpp"
#include <fstream>

json &JSONCache::getJSONDocument(std::string path, std::string name) {
  std::string fullName = path + name;

  if (JSONDocuments.find(fullName) == JSONDocuments.end()) {
    std::ifstream docStream(fullName + ".json");
    std::stringstream buffer;
    buffer << docStream.rdbuf();
    JSONDocuments.insert(std::make_pair(fullName, json::parse(buffer.str())));
  }

  return JSONDocuments[fullName];
}