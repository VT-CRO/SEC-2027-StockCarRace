#include "RelativeEncoder.hpp"
#include <limits>

float RelativeEncoder::revolutions() const {
  const int32_t countsPerRev = countsPerRevolution();
  if (countsPerRev <= 0) {
    return std::numeric_limits<float>::quiet_NaN();
  }
  return static_cast<float>(count()) / static_cast<float>(countsPerRev);
}

float RelativeEncoder::angleDegrees() const { return revolutions() * 360.0f; }

float RelativeEncoder::averageRpm(int32_t startCount, int32_t endCount,
                                  uint32_t elapsedMs) const {
  const int32_t countsPerRev = countsPerRevolution();
  if (elapsedMs == 0 || countsPerRev <= 0) {
    return std::numeric_limits<float>::quiet_NaN();
  }
  const int64_t delta = static_cast<int64_t>(endCount) - startCount;
  return static_cast<float>(delta) * 60000.0f /
         (static_cast<float>(countsPerRev) * elapsedMs);
}
