#include "MPU6050IO.h"

#include <Arduino.h>


bool MPU6050IO::begin() {
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

  values.yaw = yawPitchRoll[0]; values.pitch = yawPitchRoll[1]; values.roll = yawPitchRoll[2];
  return true;
}
