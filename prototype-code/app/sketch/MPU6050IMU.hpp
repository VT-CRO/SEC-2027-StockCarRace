#ifndef MPU6050IMU_H_
#define MPU6050IMU_H_

#include "IMU.hpp"
#include <stdint.h>

namespace MPU6050IMUConstants {
constexpr uint8_t DEFAULT_MPU_ADDR = 0x68;
constexpr uint8_t SMPLRT_DIV = 0x19;
constexpr uint8_t CONFIG = 0x1A;
constexpr uint8_t GYRO_CONFIG = 0x1B;
constexpr uint8_t ACCEL_CONFIG = 0x1C;
constexpr uint8_t PWR_MGMT_1 = 0x6B;
constexpr uint8_t ACCEL_XOUT_H = 0x3B;
constexpr uint8_t GYRO_ZOUT_H = 0x47;
// Divisor for +/-250 deg/s sensitivity
constexpr float GYRO_SCALE_250_DPS = 131.0;
// Maximum variance in rad/sec^2 for a calibration before it's rejected
constexpr float GYRO_MAX_CAL_VARIANCE = 3e-5f;
}; // namespace MPU6050IMUConstants

/**
 * Extends IMU to provide support for the MPU6050
 */
class MPU6050IMU : public IMU {
public:
  /**
   * @brief Instantiate a new MPU6050IMU with addr 0x68 and gyro scale of
   * 250deg/s
   */
  MPU6050IMU();

  /**
   * @brief Instantiate a new MPU6050IMU with a specified addr and a gyro scale
   * of 250deg/s
   * @param mpu_i2c_id The i2c ID of the MPU6050. Must equal either 0x68, and
   * will usually equal 0x68 unless configured otherwise.
   * This overload initializes with a gyro scale of 250deg/s
   */
  MPU6050IMU(uint8_t mpu_i2c_id);

  /**
   * Begins communication and initializes the MPU6050.
   * MUST BE CALLED AFTER Serial.begin and Wire.begin!!!
   */
  bool begin() override;

  bool calibrate() override;

  /**
   * @brief Manually set the bias based on an earlier calibration
   * @param bias_ The average measurement bias, in radians per second
   */
  void manuallyCalibrate(float bias_);

  bool update() override;

  float getHeadingRads() override;

  void resetHeading(float newHeading) override;

private:
  // i2c ID of the MPU.
  const uint8_t mpu_i2c_id;
  // gyroZOut / gyro_scale = angular velocity in deg/sec
  const float gyro_scale;
  float bias;

  // Microsecond timestamp of the last heading read
  // If last_read is zero, the heading has never been read and so last_read
  // should be initialized and the current read ignored, otherwise integration
  // of heading will have a massive jump after initialization.
  unsigned long last_read;

  // Current heading, [-pi, pi)
  float headingRads;

  /**
   * Print line "[MPU6050 {mpu_i2c_id}] ERROR: {message}" to serial.
   */
  void print_error(const char *message);

  /**
   * Write a byte to a register on the IMU
   */
  void writeRegister(uint8_t reg, uint8_t value);

  /**
   * Read two bytes (high byte first) and combines them into a signed value
   */
  int16_t readWord();

  /**
   * Request and reads the current angular velocity about the Z axis from the
   * gyroscope and store the result as a float in radians per second.
   *
   * Returns true if the read succeeded, false on i2c error.
   *
   * This function DOES NOT account for bias.
   *
   * This is factored out of update because it's shared with calibrate.
   */
  bool getGyroZRadPerSec(float *radPerSec);
};

#endif
