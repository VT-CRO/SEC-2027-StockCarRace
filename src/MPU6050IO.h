#ifndef MPU6050_IO_H_
#define MPU6050_IO_H_

#include <MPU6050_6Axis_MotionApps20.h>

class MPU6050IO {
public:
  struct RawValues {
    int16_t accelX;
    int16_t accelY;
    int16_t accelZ;
    int16_t gyroX;
    int16_t gyroY;
    int16_t gyroZ;
  };

  struct Orientation {
    float yaw;
    float pitch;
    float roll;
  };

  /* Wire.begin() MUST be called before begin(). */
  bool begin();

  /*
   * Figures out the offset of the accelerometer and gyroscope from the current
   * orientation. Zeros the sensor values while the robot is stationary.
   * Should take around 2-4 seconds.
   */
  bool calibrate(uint8_t loops = 6);

  /* Reads the raw/unfused accelerometer and gyroscope values which are less stable, use readOrientation instead for more stable readings. */
  bool readRaw(RawValues& values);

  /*
   * Reads yaw, pitch, and roll in radians using values fused by the DMP.
   * Use this instead of readRaw() for more stable orientation values.
   */
  bool readOrientation(Orientation& values);

private:
  MPU6050 sensor;
  bool ready = false;
  uint8_t fifoBuffer[64];
};

#endif
