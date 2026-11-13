#ifndef IMU_H_
#define IMU_H_

/*
 * IMU.hpp provides the IMU class, an abstract base class to generalize
 * IO with an IMU. Currently, it only supports reading heading values.
 */
class IMU {
public:
  /**
   * begin() begins communication with the device and initializes it,
   * returning true if communication and initialization succeeded, and false
   * otherwise.
   */
  virtual bool begin() = 0;

  /**
   * calibrate() performs necessary measurements and self-calibrates the IMU
   * object so that its measurements will be more accurate. This function MUST
   * be called only while the robot is flat and level.
   *
   * Returns true if calibration succeeded and false if it failed.
   */
  virtual bool calibrate() = 0;

  /**
   * Update the sensor reading. Return false if the read fails.
   */
  virtual bool update() = 0;

  /**
   * getHeadingRads() returns the current heading, [-pi, pi).
   */
  virtual float getHeadingRads() = 0;

  /*
   * Set the current heading; All future measurements will be relative to this
   * datapoint, until it is called again.
   */
  virtual void resetHeading(float newHeading) = 0;

  virtual ~IMU() = default;
};

#endif
