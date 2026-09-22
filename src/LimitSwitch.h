#ifndef LIMITSWITCH_H_
#define LIMITSWITCH_H_

#include "Arduino.h"
#include <avr/io.h>
#include <avr/pgmspace.h>

class LimitSwitch {
  
  public:

    LimitSwitch(int pin);

    void updateSwitch();
    bool readSwitch() const;
    bool readSwitchHold() const;

    private:

    int m_pin;

    enum State{ON , OFF, HOLD};

    double time_limit{3000};

    struct BUTTON{
      State state{OFF};
      int time{0};
      int initial_time{0};
    };

    BUTTON limit_button;

};

#endif