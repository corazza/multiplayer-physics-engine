#include <iostream>
#include <utility>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "render.hpp"

void RectTarget::render(Renderer *sdlRenderer) {
  std::pair<int, int> position = sdlRenderer->sceneToScreen(target->position());

  b2Vec2 screenDim = dim;
  screenDim *= sdlRenderer->metersToPixels;

  SDL_Rect dst;
  dst.x = std::get<0>(position) - round(screenDim.x);
  dst.y = std::get<1>(position) - round(screenDim.y);
  dst.w = round(screenDim.x * 2);
  dst.h = round(screenDim.y * 2);

  // printf("rendering rect at %d %d %dx%d t %p\n", dst.x, dst.y, dst.w, dst.h,
  // texture);

  SDL_RenderCopyEx(sdlRenderer->sdlRenderer, texture, NULL, &dst,
                   -target->angle(), NULL, SDL_FLIP_NONE);
}

void TileTarget::render(Renderer *sdlRenderer) {
  std::pair<int, int> upperLeft =
      sdlRenderer->sceneToScreen(target->position() - dim);

  SDL_Rect dst;
  dst.w = round(sdlRenderer->metersToPixels * tileDim.x * 2);
  dst.h = round(sdlRenderer->metersToPixels * tileDim.y * 2);

  for (int i = 0; i * dst.w < 2 * sdlRenderer->metersToPixels * dim.x; ++i) {
    dst.x = std::get<0>(upperLeft) + i * dst.w;
    dst.y = std::get<1>(upperLeft) + 0 * dst.h;

    SDL_RenderCopyEx(sdlRenderer->sdlRenderer, tiledWith, NULL, &dst,
                     -target->angle(), NULL, SDL_FLIP_NONE);
  }
}

Renderer::Renderer(SDL_Renderer *sdlRenderer) : sdlRenderer(sdlRenderer) {
  SDL_RenderGetLogicalSize(sdlRenderer, &screenWidth, &screenHeight);
}

SDL_Texture *Renderer::getTexture(std::string name) {
  std::string path = "res/images/" + name;
  SDL_Texture *texture = IMG_LoadTexture(sdlRenderer, path.c_str());
  textures.insert(std::make_pair(name, texture));
  return texture;
}

// only internally-created resources need to be managed
// Renderer::~Renderer() {
//   for (std::vector<RenderTarget *>::iterator i = targets.begin();
//        i != targets.end(); ++i) {
//     delete *i;
//   }
// }

b2Vec2 Renderer::screenToScene(int x, int y) {
  return b2Vec2(cameraPosition->x + (x - screenWidth / 2) / metersToPixels,
                cameraPosition->y + (y - screenHeight / 2) / metersToPixels);
}

std::pair<int, int> Renderer::sceneToScreen(b2Vec2 pos) {
  pos -= *cameraPosition;
  pos *= metersToPixels;
  return std::pair<int, int>(screenWidth / 2 + round(pos.x),
                             screenHeight / 2 + round(pos.y));
}

void Renderer::add(RenderTarget *target) { targets.push_back(target); }

void Renderer::removeAll() {
  for (std::vector<RenderTarget *>::iterator i = targets.begin();
       i != targets.end(); ++i) {
    delete *i;
  }

  targets.clear();
}

void Renderer::render() {
  SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
  SDL_RenderClear(sdlRenderer);

  for (auto &target : targets) {
    target->render(this);
  }

  SDL_RenderPresent(sdlRenderer);
}