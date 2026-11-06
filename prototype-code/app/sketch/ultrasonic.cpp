#include "ultrasonic.hpp"
#include "Arduino.h"

long readDistanceCm(uint8_t trigPin, uint8_t echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  const unsigned long echoDurationUs = pulseIn(echoPin, HIGH, 25000);
  if (echoDurationUs == 0) {
    return -1;
  }

  return echoDurationUs / 58;
}