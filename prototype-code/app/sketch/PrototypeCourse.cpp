#include "PrototypeCourse.hpp"
#include "IMU.hpp"
#include "RelativeEncoder.hpp"
#include "StraightDrive.hpp"
#include "TankDrivetrain.hpp"
#include "wrapAngle.hpp"
#include <math.h>
#include <string.h>

namespace {
// Distances refer to the drive axle midpoint.
constexpr float MM_PER_INCH = 25.4f;
constexpr float RADS_PER_DEG = static_cast<float>(M_PI) / 180.0f;
constexpr float DEG_PER_RAD = 1.0f / RADS_PER_DEG;
constexpr float ENTRY_INCHES = 62.0f; // Starting square to the first turn, once per run.
constexpr float SHORT_INCHES = 18.0f; // Distance across the short edges between turns.
constexpr float LONG_INCHES = 42.0f; // Along the long straights between turns.
constexpr unsigned COURSE_LAPS = 3;
constexpr unsigned LEGS_PER_LAP = 4;
// Minimum time between gyro samples and course control updates while driving.
constexpr unsigned long CONTROL_INTERVAL_MS = 10;
// Minimum time between Serial Monitor polls for start, straight, and stop.
constexpr unsigned long COMMAND_INTERVAL_MS = 50;
constexpr unsigned long STRAIGHT_TIMEOUT_MS = 10000;
constexpr float STRAIGHT_HEADING_LIMIT_DEG = 20.0f;

constexpr float TURN_DEG = 90.0f; // Positive turns left.
constexpr float PIVOT_LEFT = -0.18f; // Left wheel reverses during a left pivot.
constexpr float PIVOT_RIGHT = 0.30f; // Right wheel drives forward and needs more throttle.
constexpr float TURN_SLOW_ZONE_DEG = 40.0f; // Reduce throttle within this angle of the target.
constexpr float MIN_PIVOT_SCALE = 0.70f; // Keep enough throttle to turn near the target.
constexpr float TURN_TOLERANCE_DEG = 5.0f; // Accept this much error after coasting.
constexpr unsigned long TURN_TIMEOUT_MS = 4500; // Total pivot budget, including corrections.
constexpr unsigned long TRIM_PULSE_MS = 80; // Maximum powered time for one angle correction.
constexpr unsigned long SETTLE_MS = 250; // Hold neutral before checking the angle again.

constexpr StraightDriveTuning STRAIGHT_TUNING{
    0.20f, // Left base throttle before feedback.
    0.25f, // Right base throttle, higher to help it launch.
    0.25f, // Throttle correction per revolution of wheel-travel error.
    0.35f, // Desired wheel-travel difference per radian of heading error.
};
} // namespace

PrototypeCourse::PrototypeCourse(TankDrivetrain &drivetrain, RelativeEncoder &left,
                                 RelativeEncoder &right, IMU &imu, Stream &monitor,
                                 float wheelDiameterMm, bool yawReversed)
    : drivetrain(drivetrain), leftEncoder(left), rightEncoder(right),
      courseImu(imu), monitor(monitor), courseCommands(monitor),
      wheelInchesPerRev(wheelDiameterMm * static_cast<float>(M_PI) / MM_PER_INCH),
      yawSign(yawReversed ? -1.0f : 1.0f) {}

bool PrototypeCourse::initializeSensors() {
  const bool leftReady = leftEncoder.begin();
  const bool rightReady = rightEncoder.begin();
  const bool imuReady = courseImu.begin();
  ready = leftReady && rightReady && imuReady;
  return ready;
}

void PrototypeCourse::stopCourse(const char *message) {
  drivetrain.stop();
  courseState = CourseState::Idle;
  monitor.println(message);
}

void PrototypeCourse::beginCourseStraight(unsigned long now) {
  // Measure travel from this leg's starting position while retaining encoder counts.
  leftStart = leftEncoder.revolutions();
  rightStart = rightEncoder.revolutions();
  stateStarted = now;
  courseState = CourseState::Straight;
}

bool PrototypeCourse::handleCommand(unsigned long now) {
  if (now - lastCourseInput < COMMAND_INTERVAL_MS) {
    return false;
  }
  lastCourseInput = now;
  if (courseCommands.poll() != MonitorCommandReader::Result::Command) {
    return false;
  }
  const char *command = courseCommands.command();
  if (strcmp(command, "stop") == 0) {
    stopCourse("Course stopped. Send start to restart.");
    return true;
  }
  const bool straightCheck = strcmp(command, "straight") == 0;
  if (courseState != CourseState::Idle ||
      (!straightCheck && strcmp(command, "start") != 0)) {
    return false;
  }

  monitor.println("Keep the car still while the gyro calibrates.");
  if (courseImu.calibrate()) {
    courseImu.resetHeading(0.0f);
    desiredHeading = 0.0f;
    courseLeg = 0;
    enteringCourse = true;
    firstStraightOnly = straightCheck;
    discardMonitorInput(monitor);
    beginCourseStraight(millis());
  }
  return true;
}

bool PrototypeCourse::pollRunCommands() {
  return ready && handleCommand(millis());
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
    stopCourse("Course stopped: IMU read failed.");
    return false;
  }
  const float heading = courseImu.getHeadingRads() * yawSign;
  // Use the shortest signed angle across the gyro's wrap at ±180 degrees.
  yawDegrees = wrapAngle(heading - desiredHeading) * DEG_PER_RAD;
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
    stopCourse("Course stopped: straight timed out.");
  } else if (fabsf(yawDegrees) >= STRAIGHT_HEADING_LIMIT_DEG) {
    stopCourse("Course stopped: heading error reached 20 degrees.");
  } else if (distanceInches >= targetInches) {
    drivetrain.stop();
    monitor.print("Straight target reached. Estimated inches: ");
    monitor.print(distanceInches);
    monitor.print(" | Average wheel revolutions: ");
    monitor.println(wheelRevs);
    if (firstStraightOnly) {
      stopCourse("First straight complete. Both ESCs at neutral.");
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
  if (courseState == CourseState::Idle) {
    monitor.print("Estimated inches: ");
    monitor.print(distanceInches);
    monitor.print(" | Heading error (deg): ");
    monitor.println(yawDegrees);
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
    stopCourse("Course stopped: turn timeout.");
    monitor.print("Turn heading error (deg): ");
    monitor.println(yawDegrees);
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
      stopCourse("Course stopped: pivot did not settle on target.");
      monitor.print("Settled heading error (deg): ");
      monitor.println(yawDegrees);
    } else {
      // Reset the pulse timer while retaining the overall turn deadline.
      stateStarted = now;
      trimmingTurn = true;
      courseState = CourseState::Turn;
    }
    return;
  }

  monitor.print("Pivot settled. Heading error (deg): ");
  monitor.println(yawDegrees);
  // The entry pivot joins the loop. Only subsequent pivots count toward laps.
  if (enteringCourse) {
    enteringCourse = false;
  } else {
    ++courseLeg;
  }
  if (courseLeg == COURSE_LAPS * LEGS_PER_LAP) {
    stopCourse("Course complete. Send start to run again.");
  } else {
    beginCourseStraight(millis());
  }
}
