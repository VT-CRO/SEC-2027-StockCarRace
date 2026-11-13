#ifndef RELATIVE_ENCODER_H_
#define RELATIVE_ENCODER_H_

#include <cstdint>

/*
 * RelativeEncoder exposes motion measured from an arbitrary zero point.
 * Implementations own the sensor-specific IO and counting mechanism.
 */
class RelativeEncoder {
public:
  virtual bool begin() = 0;
  virtual int32_t count() const = 0;
  virtual void reset() = 0;
  virtual int32_t countsPerRevolution() const = 0;

  float revolutions() const;
  float angleDegrees() const;

  // Signed average shaft RPM between two count snapshots with the same zero.
  float averageRpm(int32_t startCount, int32_t endCount,
                   uint32_t elapsedMs) const;

  virtual ~RelativeEncoder() = default;
};

#endif
