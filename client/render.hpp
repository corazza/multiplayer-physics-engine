#ifndef GAME_RENDER
#define GAME_RENDER

#include <vector>

#include <Box2D/Box2D.h>

#include "scene.hpp"
#include "scene_objects.hpp"

struct Renderer;

struct RenderTarget {
  Object *target;
  b2Vec2 dim;

  RenderTarget(Object *target, b2Vec2 dim) : target(target), dim(dim) {}
  virtual void render(Renderer *renderer) = 0;
};

struct RectTarget : RenderTarget {
  SDL_Texture *texture;

  RectTarget(Object *target, b2Vec2 dim, SDL_Texture *texture)
      : RenderTarget(target, dim), texture(texture) {}

  void render(Renderer *renderer) override;
};

struct TileTarget : RenderTarget {
  SDL_Texture *tiledWith;
  b2Vec2 tileDim;

  TileTarget(Object *target, b2Vec2 dim, SDL_Texture *tiledWith, b2Vec2 tileDim)
      : RenderTarget(target, dim), tiledWith(tiledWith), tileDim(tileDim) {}

  void render(Renderer *renderer) override;
};

struct Renderer {
  int screenWidth, screenHeight;
  SDL_Renderer *sdlRenderer;
  std::map<std::string, SDL_Texture *> textures;
  std::vector<RenderTarget *> targets;
  b2Vec2 *cameraPosition;
  double metersToPixels = 20;

  Renderer(SDL_Renderer *renderer);
  // ~Renderer();

  SDL_Texture *getTexture(std::string resId);

  b2Vec2 screenToScene(int x, int y);
  std::pair<int, int> sceneToScreen(b2Vec2 pos);

  void add(RenderTarget *target);
  void removeAll();
  void render();
};

#endif