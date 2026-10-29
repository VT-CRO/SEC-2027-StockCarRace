#include "LimitSwitch.h"

LimitSwitch::LimitSwitch(int pin) {
  m_pin = pin;
  pinMode(m_pin, INPUT);
}

void LimitSwitch::updateSwitch(){

  switch(limit_button.state)
  {
    case ON:
      if(digitalRead(m_pin) == LOW) // limit switch unpressd
      {
        limit_button.state = OFF;
      }
      if(limit_button.time >= time_limit) // limit switch pressed for time
      {
        limit_button.state = HOLD;
      }
      limit_button.time = millis() - limit_button.initial_time;

    break;
    case OFF:
      if(digitalRead(m_pin) == HIGH) // limit switch pressed
      {
        limit_button.state = ON;
        limit_button.initial_time = millis();
      }
    break;
    case HOLD:
      if(digitalRead(m_pin) == LOW)
      {
        limit_button.state = OFF;
      }
      break;
  }

}

bool LimitSwitch::readSwitch() const {
  if(limit_button.state == ON || limit_button.state == HOLD)
  {
    return true;
  }
  return false;
}

bool LimitSwitch::readSwitchHold() const{
  if(limit_button.state == HOLD)
  {
    return true;
  }
  return false;
}
