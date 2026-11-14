#ifndef BENCH_TEST_H_
#define BENCH_TEST_H_

#include "RelativeEncoder.hpp"
#include "IMU.hpp"
#include "TankDrivetrain.hpp"

// Run once after the ESCs have had time to arm at neutral.
void runBenchTest(TankDrivetrain &drivetrain);

// Calibrate while stationary, then run a short gyro heading-hold trial.
void runHeadingBenchTest(TankDrivetrain &drivetrain, IMU &imu, bool yawReversed);

// Compare wheel-travel matching and RPM tracking, with optional gyro steering.
void runStraightBenchTest(TankDrivetrain &drivetrain, RelativeEncoder &left,
                          RelativeEncoder &right, IMU *imu, bool yawReversed);

// Wait for the operator before each wheel's one-revolution out-and-back test.
// Call after both motors and encoders initialize, with both wheels raised.
void runEncoderBenchTest(TankDrivetrain &drivetrain, RelativeEncoder &left,
                         RelativeEncoder &right);

#endif
