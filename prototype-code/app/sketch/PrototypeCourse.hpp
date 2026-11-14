#pragma once

#include <Arduino.h>

class IMU;
class RelativeEncoder;
class TankDrivetrain;

// Runs the entry straight once, then alternates loop straights and pivots.
// Initialize sensors after starting the ESC signals, Monitor, and I2C bus.
class PrototypeCourse {
public:
  PrototypeCourse(TankDrivetrain &drivetrain, RelativeEncoder &left,
                  RelativeEncoder &right, IMU &imu,
                  float wheelDiameterMm, bool yawReversed);
  enum class CourseState { Idle, Straight, Turn, Settle };

  // Require both encoders and the gyro before accepting motion commands.
  bool initializeSensors();
  // Calibrate while stationary, then start the entry straight.
  bool start(bool straightOnly);
  // Read a fresh heading at the control interval. False skips this update
  // while idle, between samples, or while holding neutral after a failed read.
  bool sampleHeadingError(unsigned long now, float &yawDegrees);
  CourseState state() const;

  // Call the handler for the current state after a successful heading sample.
  void updateStraight(unsigned long now, float yawDegrees);
  void updateTurn(unsigned long now, float yawDegrees);
  void updateSettle(unsigned long now, float yawDegrees);

private:
  void stopCourse();
  void beginCourseStraight(unsigned long now);

  TankDrivetrain &drivetrain;
  RelativeEncoder &leftEncoder;
  RelativeEncoder &rightEncoder;
  IMU &courseImu;
  const float wheelInchesPerRev;
  const float yawSign;

  CourseState courseState = CourseState::Idle;
  bool ready = false;
  bool firstStraightOnly = false;
  bool enteringCourse = true;
  bool trimmingTurn = false;
  unsigned int courseLeg = 0; // Completed loop legs. The entry is separate.
  unsigned long stateStarted = 0;
  unsigned long turnStarted = 0; // Start time for the pivot and all corrections.
  unsigned long lastCourseUpdate = 0;
  float desiredHeading = 0.0f; // Radians, matching the IMU interface.
  float leftStart = 0.0f;
  float rightStart = 0.0f;
};
