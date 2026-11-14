#pragma once

class TankDrivetrain;

struct StraightDriveTuning {
  float leftBase;  // Left normalized throttle before steering correction.
  float rightBase; // Right normalized throttle before steering correction.
  float wheelKp;   // Normalized throttle correction per revolution of gap error.
  float yawKp;     // Desired wheel-travel difference in revolutions per radian.
};

// Apply one feedback update. The caller supplies wheel travel and yaw relative
// to the current straight and calls stop() on the drivetrain when finished.
// wheelGap is left travel minus right travel in revolutions, measured from
// the start of the straight. yaw is the wrapped heading error in radians,
// with positive yaw meaning the car points left of its desired heading.
// Pass zero yaw to use encoder-only travel matching.
void driveStraight(TankDrivetrain &drivetrain, const StraightDriveTuning &tuning,
                   float wheelGap, float yaw);
