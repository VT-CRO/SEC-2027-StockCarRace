#ifndef ULTRASONIC_H_
#define ULTRASONIC_H_

#include <stdint.h>

long readDistanceCm(uint8_t trig_pin, uint8_t echo_pin);

#endif