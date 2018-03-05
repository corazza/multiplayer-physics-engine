#include <iostream>
#include <utility>

#include <unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "render.hpp"

double radiansToDegrees(float32 radians) {
  return -int(round(180 * radians / M_PI)) % 360;
}

void RectTarget::render(Renderer *sdlRenderer) {
  std::pair<int, int> position =
      sdlRenderer->sceneToScreen(target->body->GetPosition());

  b2Vec2 screenDim = dim;
  screenDim *= sdlRenderer->metersToPixels;

  SDL_Rect dst;
  dst.x = std::get<0>(position) - round(screenDim.x);
  dst.y = std::get<1>(position) - round(screenDim.y);
  dst.w = round(screenDim.x * 2);
  dst.h = round(screenDim.y * 2);

  SDL_RenderCopyEx(sdlRenderer->sdlRenderer, texture, NULL, &dst,
                   -radiansToDegrees(target->body->GetAngle()), NULL,
                   SDL_FLIP_NONE);
}

void TileTarget::render(Renderer *sdlRenderer) {
  std::pair<int, int> upperLeft =
      sdlRenderer->sceneToScreen(target->body->GetPosition() - dim);

  SDL_Rect dst;
  dst.w = round(sdlRenderer->metersToPixels * tileDim.x * 2);
  dst.h = round(sdlRenderer->metersToPixels * tileDim.y * 2);

  for (int i = 0; i * dst.w < 2 * sdlRenderer->metersToPixels * dim.x; ++i) {
    dst.x = std::get<0>(upperLeft) + i * dst.w;
    dst.y = std::get<1>(upperLeft) + 0 * dst.h;

    SDL_RenderCopyEx(sdlRenderer->sdlRenderer, tiledWith, NULL, &dst,
                     -radiansToDegrees(target->body->GetAngle()), NULL,
                     SDL_FLIP_NONE);
  }
}

Renderer::Renderer(SDL_Renderer *sdlRenderer) : sdlRenderer(sdlRenderer) {
  SDL_RenderGetLogicalSize(sdlRenderer, &screenWidth, &screenHeight);
}

Renderer::~Renderer() {
  removeAll();

  for (auto texture : textures) {
    SDL_DestroyTexture(texture.second);
  }
}

SDL_Texture *Renderer::getTexture(std::string name) {
  std::string path = "res/images/" + name;

  auto texture_it = textures.find(name);

  if (texture_it == textures.end()) {
    SDL_Texture *texture = IMG_LoadTexture(sdlRenderer, path.c_str());

    textures.insert(std::make_pair(name, texture));
    return texture;
  } else {
    return texture_it->second;
  }
}

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

void Renderer::add(RenderTarget *target) { newTargets.push_back(target); }

void Renderer::remove(Object *object) { targets.erase(object); }

void Renderer::removeAll() {
  for (auto target : targets) {
    delete target.second;
  }

  targets.clear();
}

void Renderer::render() {
  SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
  SDL_RenderClear(sdlRenderer);

  for (auto target : targets) {
    target.second->render(this);
  }

  auto newTargetsCopy = newTargets;

  for (auto newTarget : newTargetsCopy) {
    targets.insert(std::make_pair(newTarget->target, newTarget));
  }

  newTargets.clear();

  SDL_RenderPresent(sdlRenderer);
  usleep(5000);
}