#ifndef ARDUINO_QUADRATURE_ENCODER_H_
#define ARDUINO_QUADRATURE_ENCODER_H_

#include "RelativeEncoder.hpp"
#include <Arduino.h>
#include <atomic>

/*
 * Four-edge quadrature decoder for an encoder with A and B outputs.
 * The encoder's electrical details stop at this class; users depend only on
 * RelativeEncoder. This adapter currently supports one encoder instance,
 * because Arduino's attachInterrupt API does not provide a callback context.
 * The instance must have static storage duration and must not be destroyed
 * while interrupts may still be delivered.
 */
class ArduinoQuadratureEncoder : public RelativeEncoder {
public:
  ArduinoQuadratureEncoder(uint8_t channelAPin, uint8_t channelBPin,
                           int32_t countsPerRevolution,
                           bool reversed = false);
  ~ArduinoQuadratureEncoder() override;

  ArduinoQuadratureEncoder(const ArduinoQuadratureEncoder &) = delete;
  ArduinoQuadratureEncoder &
  operator=(const ArduinoQuadratureEncoder &) = delete;

  bool begin() override;
  int32_t count() const override;
  void reset() override;
  int32_t countsPerRevolution() const override;

private:
  static ArduinoQuadratureEncoder *instance_;

  static void interruptHandler();
  void handleInterrupt();

  const uint8_t channelAPin_;
  const uint8_t channelBPin_;
  const int32_t countsPerRevolution_;
  const bool reversed_;
  // Position wraps modulo 2^32: INT32_MAX is followed by INT32_MIN.
  std::atomic<int32_t> count_{0};
  volatile uint8_t previousState_ = 0;
  bool started_ = false;
};

#endif
