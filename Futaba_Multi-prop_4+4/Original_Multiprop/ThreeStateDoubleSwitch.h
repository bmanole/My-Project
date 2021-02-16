#ifndef __THREE_STATE_DOUBLE_SWITCH_H
#define __THREE_STATE_DOUBLE_SWITCH_H

#include <Arduino.h>

class ThreeStateDoubleSwitch {
  private:
    uint8_t prevState = 1;
    uint8_t currentState = 1;
    boolean upperSwitchOn = false;
    boolean lowerSwitchOn = false;
  public:
    ThreeStateDoubleSwitch();
    void computeNewState(int pulseLength);

    boolean isUpperSwitchOn();
    boolean isLowerSwitchOn();
};

#endif