#include "PrototypeCourse.hpp"
#include "IMU.hpp"
#include "RelativeEncoder.hpp"
#include "StraightDrive.hpp"
#include "TankDrivetrain.hpp"
#include "wrapAngle.hpp"
#include <math.h>

namespace {
// Distances refer to the drive axle midpoint.
constexpr float MM_PER_INCH = 25.4f;
constexpr float RADS_PER_DEG = static_cast<float>(M_PI) / 180.0f;
constexpr float DEG_PER_RAD = 1.0f / RADS_PER_DEG;
constexpr float ENTRY_INCHES = 61.0f; // Starting square to the first turn, once per run.
constexpr float SHORT_INCHES = 16.5f; // Distance across the short edges between turns.
constexpr float LONG_INCHES = 41.5f; // Along the long straights between turns.
constexpr unsigned COURSE_LAPS = 3;
constexpr unsigned LEGS_PER_LAP = 4;
// Minimum time between gyro samples and course control updates while driving.
constexpr unsigned long CONTROL_INTERVAL_MS = 10;
constexpr unsigned long STRAIGHT_TIMEOUT_MS = 10000;
constexpr float STRAIGHT_HEADING_LIMIT_DEG = 20.0f;

constexpr float TURN_DEG = 90.0f; // Positive turns left.
constexpr float PIVOT_LEFT = -0.18f; // Left wheel reverses during a left pivot.
constexpr float PIVOT_RIGHT = 0.30f; // Right wheel drives forward and needs more throttle.
constexpr float TURN_SLOW_ZONE_DEG = 40.0f; // Reduce throttle within this angle of the target.
constexpr float MIN_PIVOT_SCALE = 0.70f; // Keep enough throttle to turn near the target.
constexpr float TURN_TOLERANCE_DEG = 5.0f; // Accept this much error after coasting.
// Abort a left pivot if the chassis passes its target by this angle.
constexpr float MAX_PIVOT_OVERSHOOT_DEG = 30.0f;
constexpr unsigned long TURN_TIMEOUT_MS = 8000; // Total pivot budget, including corrections.
constexpr unsigned long TRIM_PULSE_MS = 80; // Maximum powered time for one angle correction.
constexpr unsigned long SETTLE_MS = 250; // Hold neutral before checking the angle again.

constexpr StraightDriveTuning STRAIGHT_TUNING{
    0.20f, // Left base throttle before feedback.
    0.25f, // Right base throttle, higher to help it launch.
    0.23f, // Throttle correction per revolution of wheel-travel error.
    0.29f, // Desired wheel-travel difference per radian of heading error.
};
} // namespace

PrototypeCourse::PrototypeCourse(TankDrivetrain &drivetrain, RelativeEncoder &left,
                                 RelativeEncoder &right, IMU &imu,
                                 float wheelDiameterMm, bool yawReversed)
    : drivetrain(drivetrain), leftEncoder(left), rightEncoder(right),
      courseImu(imu),
      wheelInchesPerRev(wheelDiameterMm * static_cast<float>(M_PI) / MM_PER_INCH),
      yawSign(yawReversed ? -1.0f : 1.0f) {}

bool PrototypeCourse::initializeSensors() {
  const bool leftReady = leftEncoder.begin();
  const bool rightReady = rightEncoder.begin();
  const bool imuReady = courseImu.begin();
  ready = leftReady && rightReady && imuReady;
  return ready;
}

void PrototypeCourse::stopCourse() {
  drivetrain.stop();
  courseState = CourseState::Idle;
}

void PrototypeCourse::beginCourseStraight(unsigned long now) {
  // Measure travel from this leg's starting position while retaining encoder counts.
  leftStart = leftEncoder.revolutions();
  rightStart = rightEncoder.revolutions();
  stateStarted = now;
  courseState = CourseState::Straight;
}

bool PrototypeCourse::start(bool straightOnly) {
  if (!ready || courseState != CourseState::Idle || !courseImu.calibrate()) {
    return false;
  }
  courseImu.resetHeading(0.0f);
  desiredHeading = 0.0f;
  courseLeg = 0;
  enteringCourse = true;
  firstStraightOnly = straightOnly;
  lastCourseUpdate = millis();
  beginCourseStraight(lastCourseUpdate);
  return true;
}

PrototypeCourse::CourseState PrototypeCourse::state() const {
  return courseState;
}

bool PrototypeCourse::sampleHeadingError(unsigned long now, float &yawDegrees) {
  if (!ready || courseState == CourseState::Idle) {
    return false;
  }
  if (now - lastCourseUpdate < CONTROL_INTERVAL_MS) {
    return false;
  }
  lastCourseUpdate = now;
  if (!courseImu.update()) {
    // Hold neutral while waiting for fresh heading data. Leave the segment
    // active so the next control update can retry the read.
    drivetrain.stop();
    return false;
  }
  const float heading = courseImu.getHeadingRads() * yawSign;
  // Use the shortest signed angle across the gyro's wrap at ±180 degrees.
  yawDegrees = wrapAngle(heading - desiredHeading) * DEG_PER_RAD;
  // The course pivots left, so positive error means it passed the target.
  if ((courseState == CourseState::Turn || courseState == CourseState::Settle) &&
      yawDegrees >= MAX_PIVOT_OVERSHOOT_DEG) {
    stopCourse();
    return false;
  }
  return true;
}

// Estimate axle travel from the average encoder change since this leg began.
// Steer from wheel travel and yaw until the entry or loop distance is reached.
// Stop on timeout or excessive yaw. At the endpoint, either finish the
// straight-only run or hold neutral and set up the next pivot.
void PrototypeCourse::updateStraight(unsigned long now, float yawDegrees) {
  const unsigned long elapsed = now - stateStarted;
  const float leftTravel = leftEncoder.revolutions() - leftStart;
  const float rightTravel = rightEncoder.revolutions() - rightStart;
  const float wheelRevs = (leftTravel + rightTravel) * 0.5f;
  const float distanceInches = wheelRevs * wheelInchesPerRev;
  // The launch runs once. After its pivot, even-numbered loop legs cross
  // the track and odd-numbered legs run along its length.
  const float targetInches = enteringCourse ? ENTRY_INCHES :
      (courseLeg % 2 == 0 ? SHORT_INCHES : LONG_INCHES);
  if (elapsed >= STRAIGHT_TIMEOUT_MS) {
    stopCourse();
  } else if (fabsf(yawDegrees) >= STRAIGHT_HEADING_LIMIT_DEG) {
    stopCourse();
  } else if (distanceInches >= targetInches) {
    drivetrain.stop();
    if (firstStraightOnly) {
      stopCourse();
      return;
    }
    // Advance from the intended heading so turn errors don't accumulate each lap.
    desiredHeading = wrapAngle(desiredHeading + TURN_DEG * RADS_PER_DEG);
    stateStarted = millis();
    turnStarted = stateStarted;
    trimmingTurn = false;
    courseState = CourseState::Turn;
  } else {
    driveStraight(drivetrain, STRAIGHT_TUNING, leftTravel - rightTravel, yawDegrees * RADS_PER_DEG);
  }
}

// Steer toward the target heading, slowing the initial pivot near the target.
// Switch to neutral and settle when within tolerance, after the initial turn
// crosses the target, or when a correction pulse ends. The overall turn
// timeout includes all correction pulses and neutral pauses.
void PrototypeCourse::updateTurn(unsigned long now, float yawDegrees) {
  const unsigned long elapsed = now - stateStarted;
  const float remaining = fabsf(yawDegrees);
  if (now - turnStarted >= TURN_TIMEOUT_MS) {
    stopCourse();
  } else if (remaining <= TURN_TOLERANCE_DEG ||
             (!trimmingTurn && yawDegrees > 0.0f) ||
             (trimmingTurn && elapsed >= TRIM_PULSE_MS)) {
    drivetrain.stop();
    stateStarted = now;
    courseState = CourseState::Settle;
  } else {
    // The minimum scale keeps the pivot command above the ESC deadband.
    const float scale = trimmingTurn ? MIN_PIVOT_SCALE :
        constrain(remaining / TURN_SLOW_ZONE_DEG, MIN_PIVOT_SCALE, 1.0f);
    // A negative error calls for the configured left pivot.
    const float direction = yawDegrees < 0.0f ? 1.0f : -1.0f;
    drivetrain.runOpenLoop(direction * PIVOT_LEFT * scale,
                           direction * PIVOT_RIGHT * scale);
  }
}

// Let the chassis coast at neutral before checking the heading again.
// If the error is within tolerance, start the next leg or finish the course.
// Otherwise, schedule a short correction or stop if the turn timed out.
void PrototypeCourse::updateSettle(unsigned long now, float yawDegrees) {
  if (now - stateStarted < SETTLE_MS) {
    return;
  }
  if (fabsf(yawDegrees) > TURN_TOLERANCE_DEG) {
    if (now - turnStarted >= TURN_TIMEOUT_MS) {
      stopCourse();
    } else {
      // Reset the pulse timer while retaining the overall turn deadline.
      stateStarted = now;
      trimmingTurn = true;
      courseState = CourseState::Turn;
    }
    return;
  }

  // The entry pivot joins the loop. Only subsequent pivots count toward laps.
  if (enteringCourse) {
    enteringCourse = false;
  } else {
    ++courseLeg;
  }
  if (courseLeg == COURSE_LAPS * LEGS_PER_LAP) {
    stopCourse();
  } else {
    beginCourseStraight(millis());
  }
}
