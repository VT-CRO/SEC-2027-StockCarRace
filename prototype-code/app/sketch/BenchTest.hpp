#ifndef BENCH_TEST_H_
#define BENCH_TEST_H_

#include "RelativeEncoder.hpp"
#include "TankDrivetrain.hpp"

// Run once after the ESCs have had time to arm at neutral.
void runBenchTest(TankDrivetrain &drivetrain);

// Wait for the operator before each wheel's one-revolution out-and-back test.
// Call after both motors and encoders initialize, with both wheels raised.
void runEncoderBenchTest(TankDrivetrain &drivetrain, RelativeEncoder &left,
                         RelativeEncoder &right);

#endif
