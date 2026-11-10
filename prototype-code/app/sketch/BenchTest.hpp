#ifndef BENCH_TEST_H_
#define BENCH_TEST_H_

#include "RelativeEncoder.hpp"
#include "TankDrivetrain.hpp"

// Run once after the ESCs have had time to arm at neutral.
void runBenchTest(TankDrivetrain &drivetrain);

// Run in loop after calling begin in init to test encoder:
// Reads the encoder value and waits one second
// Make sure to disable motor code and then turn the encoder by hand to test.
void testEncoder(RelativeEncoder &encoder);

#endif
