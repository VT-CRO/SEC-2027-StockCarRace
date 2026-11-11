#include "MPU6050IMU.hpp"
#include "wrapAngle.hpp"
#include <Arduino.h>
#include <Arduino_RouterBridge.h>
#include <Wire.h>
#include <cmath>
#include <math.h>

MPU6050IMU::MPU6050IMU()
    : mpu_i2c_id(MPU6050IMUConstants::DEFAULT_MPU_ADDR),
      gyro_scale(MPU6050IMUConstants::GYRO_SCALE_250_DPS), bias(0.0f),
      last_read(0), headingRads(0.0f) {}

MPU6050IMU::MPU6050IMU(uint8_t mpu_i2c_id)
    : mpu_i2c_id(mpu_i2c_id),
      gyro_scale(MPU6050IMUConstants::GYRO_SCALE_250_DPS), bias(0.0f),
      last_read(0), headingRads(0.0f) {}

bool MPU6050IMU::begin() {
  Wire.beginTransmission(mpu_i2c_id);
  if (Wire.endTransmission() != 0) {
    print_error("Device not found. Check wiring and address.");
    return false;
  }

  writeRegister(MPU6050IMUConstants::PWR_MGMT_1, 0x80); // reset
  delay(100);
  writeRegister(MPU6050IMUConstants::PWR_MGMT_1, 0x01); // wake, X-gyro PLL
  delay(10);
  writeRegister(MPU6050IMUConstants::CONFIG, 0x03); // DLPF ~42 Hz, 1 kHz output
  writeRegister(MPU6050IMUConstants::GYRO_CONFIG, 0x00); // ±250 dps
  writeRegister(MPU6050IMUConstants::SMPLRT_DIV, 0x04);  // 200 Hz

  // Make sure to mark that we've never read heading
  last_read = 0;

  Monitor.print("[MPU6050 ");
  Monitor.print(mpu_i2c_id, HEX);
  Monitor.println("] Initialized Successfully.");

  return true;
}

bool MPU6050IMU::calibrate() {
  constexpr int N = 400;
  float sum = 0.0f, sumSq = 0.0f;
  int n = 0;
  for (int i = 0; i < N; i++) {
    float w;
    if (getGyroZRadPerSec(&w)) {
      sum += w;
      sumSq += w * w;
      n++;
    }
    delay(5); // matches 200 Hz output; delay(2) re-reads duplicate samples
  }
  if (n < N * 9 / 10) {
    print_error("Too many failed reads.");
    return false;
  }
  float mean = sum / n;
  float var = sumSq / n - mean * mean;
  if (var > MPU6050IMUConstants::GYRO_MAX_CAL_VARIANCE) {
    print_error("Robot moving during calibration.");
    return false;
  }
  bias = mean;
  last_read = 0;
  headingRads = 0.0f;

  Monitor.print("[MPU6050 ");
  Monitor.print(mpu_i2c_id, HEX);
  Monitor.print("] Calibrated with bias: ");
  Monitor.print(bias);
  Monitor.println(" rad/s average.");

  return true;
}

void MPU6050IMU::manuallyCalibrate(float bias_) { bias = bias_; }

void MPU6050IMU::update() {
  if (last_read == 0) {
    // On the very first call to update, just initialize the last_read time.
    // We can't integrate yet because we don't have a real delta time.
    last_read = micros();
    return;
  }

  // Initialize now before the i2c read to avoid extra delay
  unsigned long now = micros();

  // Get angular velocity
  float gyroZRadPerSec;
  bool success = getGyroZRadPerSec(&gyroZRadPerSec);

  // Don't integrate with garbage data from i2c error
  if (!success)
    return;

  // Integrate and add to heading
  float dt = (now - last_read) * 1e-6f;
  last_read = now;
  headingRads += (gyroZRadPerSec - bias) * dt;

  // Normalize heading
  headingRads = wrapAngle(headingRads);
}

float MPU6050IMU::getHeadingRads() { return headingRads; }

void MPU6050IMU::resetHeading(float newHeading) {
  headingRads = wrapAngle(newHeading);
}

void MPU6050IMU::print_error(const char *message) {
  Monitor.print("[MPU6050 ");
  Monitor.print(mpu_i2c_id, HEX);
  Monitor.print("] ERROR: ");
  Monitor.println(message);
}

void MPU6050IMU::writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(mpu_i2c_id);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission(true);
}

int16_t MPU6050IMU::readWord() {
  int16_t high_byte = Wire.read();
  int16_t low_byte = Wire.read();
  return (high_byte << 8) | low_byte;
}

bool MPU6050IMU::getGyroZRadPerSec(float *radPerSec) {
  Wire.beginTransmission(mpu_i2c_id);
  // Start reading at the GYRO_ZOUT register
  Wire.write(MPU6050IMUConstants::GYRO_ZOUT_H);
  if (Wire.endTransmission(false) != 0)
    return false;

  // Request the two bytes of the z value
  if (Wire.requestFrom(mpu_i2c_id, (uint8_t)2, (uint8_t)1) != 2)
    return false;

  *radPerSec = readWord() / gyro_scale * (float)M_PI / 180.0f;

  return true;
}