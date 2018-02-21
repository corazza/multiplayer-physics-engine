#include "scene_objects.hpp"

double radiansToDegrees(float32 radians) {
  return -int(round(180 * radians / M_PI)) % 360;
}

Box2DObject::Box2DObject(b2Body *body) : body(body) {
  body->SetUserData(this);
}

b2Vec2 Box2DObject::position() { return body->GetPosition(); }

double Box2DObject::angle() { return radiansToDegrees(body->GetAngle()); }