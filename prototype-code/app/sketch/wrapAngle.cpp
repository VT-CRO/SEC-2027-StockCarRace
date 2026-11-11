#include "wrapAngle.hpp"
#include <cmath>

constexpr float PI_F = 3.14159265358979f;

float wrapAngle(float angle) {
  angle = fmodf(angle + PI_F, 2.0f * PI_F);
  if (angle < 0.0f) {
    angle += 2.0f * PI_F;
  }
  angle -= PI_F;

  return angle;
}