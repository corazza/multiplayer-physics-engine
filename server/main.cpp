#include "server.hpp"

int main() {
  std::ifstream confStream("serverdata/conf.json");
  std::stringstream buffer;
  buffer << confStream.rdbuf();
  auto conf = json::parse(buffer.str());
  GameServer server(conf);
}