#include "StraightDrive.hpp"
#include "TankDrivetrain.hpp"
#include <Arduino.h>

void driveStraight(TankDrivetrain &drivetrain, const StraightDriveTuning &tuning,
                   float wheelGap, float yaw) {
  constexpr float MAX_TRIM = 0.10f;
  constexpr float MAX_THROTTLE = 0.45f;

  // If the car points left of target, ask the left wheel to travel farther
  // than the right wheel to steer back right. At zero yaw, aim for equal travel.
  const float targetGap = tuning.yawKp * yaw;

  // A positive gap error means the left wheel is behind its travel target
  // relative to the right. Add throttle on the left and subtract on the right.
  const float trim = constrain(tuning.wheelKp * (targetGap - wheelGap),
                               -MAX_TRIM, MAX_TRIM);

  // Keep both wheel commands in the forward range and cap the throttle.
  drivetrain.runOpenLoop(constrain(tuning.leftBase + trim, 0.0f, MAX_THROTTLE),
                         constrain(tuning.rightBase - trim, 0.0f, MAX_THROTTLE));
}
