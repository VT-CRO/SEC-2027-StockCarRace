#ifndef TOF_H_
#define TOF_H_

#include <VL53L0X.h>

class ToF {
public:
  /* Wire.begin() MUST be called before begin(). */
  bool begin();
  double readDistanceCm();

private:
  VL53L0X sensor;
};

#endif
