#include "scene_objects.hpp"

bool Controller::active() {
  return movingLeft || movingRight || jumping || stopping;
}

b2Vec2 Object::position() {
  if (body != nullptr)
    return body->GetPosition();
  if (fixedPosition != nullptr)
    return fixedPosition->pos;
}

float32 Object::angle() {
  if (body != nullptr)
    return body->GetAngle();
  if (fixedPosition != nullptr)
    return fixedPosition->angle;
}

bool Object::resting() {
  if (body != nullptr)
    return body->GetLinearVelocity().LengthSquared() == 0;
  else
    return true;
}