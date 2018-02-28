#include "scene_objects.hpp"

bool Controller::active() {
  return movingLeft || movingRight || jumping || stopping;
}

Object::Object(b2Body *body) : body(body) { body->SetUserData(this); }

bool Object::resting() {
  return body->GetLinearVelocity().LengthSquared() == 0;
}