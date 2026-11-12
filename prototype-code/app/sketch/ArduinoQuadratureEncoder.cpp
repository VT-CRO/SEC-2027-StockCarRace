#include "ArduinoQuadratureEncoder.hpp"

namespace {
/*
Each encoder edge produces a transition from a previous two-bit state
[previous A][previous B] to a current two-bit state [A][B]. The transition
forms a four-bit table index:

    [previous A][previous B][A][B]

One possible phase relationship is:

    Positive direction -->

    A _--__--__
    B __--__--_

    <-- Negative direction

For example, a transition from A=0, B=0 to A=1, B=0 produces the index
0b0010 (2). The value at that index is +1, so this transition increments the
relative count. A valid quadrature sequence has four transitions per
electrical cycle; invalid transitions, such as both signals changing at once,
produce zero.

The signs define the encoder's positive direction; they do not universally
mean clockwise because that depends on the encoder wiring and mechanical
orientation. `reversed_` can invert the signs when the desired positive
direction is opposite.
*/
constexpr int8_t QUADRATURE_TRANSITIONS[16] = {
    0,  -1, 1,  0,  // Previous state 00: current state 00, 01, 10, 11
    1,  0,  0,  -1, // Previous state 01: current state 00, 01, 10, 11
    -1, 0,  0,  1,  // Previous state 10: current state 00, 01, 10, 11
    0,  1,  -1, 0,  // Previous state 11: current state 00, 01, 10, 11
};
} // namespace

ArduinoQuadratureEncoder *ArduinoQuadratureEncoder::instances_
    [ArduinoQuadratureEncoder::MAX_ENCODERS] = {nullptr, nullptr};

ArduinoQuadratureEncoder::ArduinoQuadratureEncoder(uint8_t channelAPin,
                                                   uint8_t channelBPin,
                                                   int32_t countsPerRevolution,
                                                   bool reversed)
    : channelAPin_(channelAPin), channelBPin_(channelBPin),
      countsPerRevolution_(countsPerRevolution), reversed_(reversed) {}

ArduinoQuadratureEncoder::~ArduinoQuadratureEncoder() {
  if (started_) {
    detachInterrupt(channelAInterrupt_);
    detachInterrupt(channelBInterrupt_);
    instances_[instanceSlot_] = nullptr;
  }
}

bool ArduinoQuadratureEncoder::begin() {
  const int channelAInterrupt = digitalPinToInterrupt(channelAPin_);
  const int channelBInterrupt = digitalPinToInterrupt(channelBPin_);

  if (channelAPin_ == channelBPin_ || countsPerRevolution_ <= 0 ||
      channelAInterrupt < 0 || channelBInterrupt < 0 ||
      channelAInterrupt == channelBInterrupt) {
    return false;
  }

  if (started_) {
    return true;
  }

  for (uint8_t slot = 0; slot < MAX_ENCODERS; ++slot) {
    if (instances_[slot] == nullptr) {
      instanceSlot_ = static_cast<int8_t>(slot);
      break;
    }
  }

  if (instanceSlot_ < 0) {
    return false;
  }

  instances_[instanceSlot_] = this;
  channelAInterrupt_ = static_cast<int8_t>(channelAInterrupt);
  channelBInterrupt_ = static_cast<int8_t>(channelBInterrupt);
  pinMode(channelAPin_, INPUT);
  pinMode(channelBPin_, INPUT);
  previousState_ = (digitalRead(channelAPin_) << 1) | digitalRead(channelBPin_);
  reset();

  void (*handler)() = instanceSlot_ == 0 ? interrupt0 : interrupt1;
  attachInterrupt(channelAInterrupt_, handler, CHANGE);
  attachInterrupt(channelBInterrupt_, handler, CHANGE);
  started_ = true;
  return true;
}

int32_t ArduinoQuadratureEncoder::count() const {
  return count_.load(std::memory_order_relaxed);
}

void ArduinoQuadratureEncoder::reset() {
  count_.store(0, std::memory_order_relaxed);
}

int32_t ArduinoQuadratureEncoder::countsPerRevolution() const {
  return countsPerRevolution_;
}

void ArduinoQuadratureEncoder::dispatch(uint8_t interruptNumber) {
  if (instances_[interruptNumber] != nullptr) {
    instances_[interruptNumber]->handleInterrupt();
  }
}

#define DEFINE_INTERRUPT_HANDLER(number)                                       \
  void ArduinoQuadratureEncoder::interrupt##number() { dispatch(number); }

DEFINE_INTERRUPT_HANDLER(0)
DEFINE_INTERRUPT_HANDLER(1)

#undef DEFINE_INTERRUPT_HANDLER

void ArduinoQuadratureEncoder::handleInterrupt() {
  const uint8_t currentState =
      (digitalRead(channelAPin_) << 1) | digitalRead(channelBPin_);
  const uint8_t transition = (previousState_ << 2) | currentState;
  const int8_t direction = QUADRATURE_TRANSITIONS[transition];
  const int32_t signedDelta = reversed_ ? -direction : direction;
  count_.fetch_add(signedDelta, std::memory_order_relaxed);
  previousState_ = currentState;
}
