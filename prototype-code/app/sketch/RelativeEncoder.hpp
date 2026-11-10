#ifndef RELATIVE_ENCODER_H_
#define RELATIVE_ENCODER_H_

#include <cstdint>
#include <limits>

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

  float revolutions() const {
    if (countsPerRevolution() <= 0) {
      return std::numeric_limits<float>::quiet_NaN();
    }

    return static_cast<float>(count()) /
           static_cast<float>(countsPerRevolution());
  }

  float degrees() const { return revolutions() * 360.0f; }

  virtual ~RelativeEncoder() = default;
};

#endif
