#pragma once

#include "MonitorInput.hpp"

class IMU;
class RelativeEncoder;
class TankDrivetrain;

// Runs the entry straight once, then alternates loop straights and pivots.
// Initialize sensors after starting the ESC signals, Monitor, and I2C bus.
class PrototypeCourse {
public:
  PrototypeCourse(TankDrivetrain &drivetrain, RelativeEncoder &left,
                  RelativeEncoder &right, IMU &imu, Stream &monitor,
                  float wheelDiameterMm, bool yawReversed);
  enum class CourseState { Idle, Straight, Turn, Settle };

  // Require both encoders and the gyro before accepting motion commands.
  bool initializeSensors();
  // True when start, straight, or stop consumed this pass through loop().
  bool pollRunCommands();
  // Read a fresh heading at the control interval. False skips this update
  // while idle, between samples, or after a failed read has stopped the car.
  bool sampleHeadingError(unsigned long now, float &yawDegrees);
  CourseState state() const;

  // Call the handler for the current state after a successful heading sample.
  void updateStraight(unsigned long now, float yawDegrees);
  void updateTurn(unsigned long now, float yawDegrees);
  void updateSettle(unsigned long now, float yawDegrees);

private:
  bool handleCommand(unsigned long now);
  void stopCourse(const char *message);
  void beginCourseStraight(unsigned long now);

  TankDrivetrain &drivetrain;
  RelativeEncoder &leftEncoder;
  RelativeEncoder &rightEncoder;
  IMU &courseImu;
  Stream &monitor;
  MonitorCommandReader courseCommands;
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
  unsigned long lastCourseInput = 0;
  float desiredHeading = 0.0f; // Radians, matching the IMU interface.
  float leftStart = 0.0f;
  float rightStart = 0.0f;
};
