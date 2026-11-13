#include "MPU6050IO.h"

#include <Arduino.h>


bool MPU6050IO::begin() {
  offset = {0.0f, 0.0f, 0.0f};
  sensor.initialize();
  if (!sensor.testConnection() || sensor.dmpInitialize() != 0) {
    ready = false;
    return false;
  }

  sensor.setDMPEnabled(true);
  ready = true;
  return true;
}


bool MPU6050IO::calibrate(uint8_t loops) {
  if (!ready || loops == 0) {
    return false;
  }

  sensor.setDMPEnabled(false);
  sensor.CalibrateAccel(loops);
  sensor.CalibrateGyro(loops);
  sensor.resetFIFO();
  sensor.setDMPEnabled(true);
  return true;
}

bool MPU6050IO::readRaw(RawValues& values) {
  if (!ready) {
    return false;
  }

  sensor.getMotion6(&values.accelX, &values.accelY, &values.accelZ,
                    &values.gyroX, &values.gyroY, &values.gyroZ);
  return true;
}


bool MPU6050IO::readOrientation(Orientation& values) {
  if (!ready || !sensor.dmpGetCurrentFIFOPacket(fifoBuffer)) {
    return false;
  }

  Quaternion quaternion;
  VectorFloat gravity;
  float yawPitchRoll[3];

  sensor.dmpGetQuaternion(&quaternion, fifoBuffer);
  sensor.dmpGetGravity(&gravity, &quaternion);
  sensor.dmpGetYawPitchRoll(yawPitchRoll, &quaternion, &gravity);

  values.yaw = yawPitchRoll[0] - offset.yaw;
  while (values.yaw > PI) {
    values.yaw -= 2.0f * PI;
  }
  while (values.yaw < -PI) {
    values.yaw += 2.0f * PI;
  }
  values.pitch = yawPitchRoll[1] - offset.pitch;
  values.roll = yawPitchRoll[2] - offset.roll;
  return true;
}

bool MPU6050IO::reset() {
  Orientation current;
  if (!readOrientation(current)) {
    return false;
  }

  offset.yaw += current.yaw;
  offset.pitch += current.pitch;
  offset.roll += current.roll;
  return true;
}
