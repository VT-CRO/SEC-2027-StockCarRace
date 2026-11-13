#include "ToF.h"

#include <Wire.h>



bool ToF::begin() {
  sensor.setTimeout(500);
  if (!sensor.init()) {
    return false;
  }

  sensor.startContinuous();
  return true;
}

double ToF::readDistanceCm() {
  return sensor.readRangeContinuousMillimeters() * 0.1;
}
