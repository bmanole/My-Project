#include "ThreeStateDoubleSwitch.h"

ThreeStateDoubleSwitch::ThreeStateDoubleSwitch() {
}

void ThreeStateDoubleSwitch::computeNewState(int pulseLength) {
  // check if upper position
  if(pulseLength >= 1020 && pulseLength <= 1070)
  {
      currentState = 2;
      if(currentState != prevState)
      {
          upperSwitchOn = !upperSwitchOn;
          prevState = currentState;
      }
  }

  // check if lower position
  if(pulseLength >= 1940 && pulseLength <= 1980)
  {
      currentState = 0;
      if(currentState != prevState)
      {
          lowerSwitchOn = !lowerSwitchOn;
          prevState = currentState;
      }
  }

  // check if middle position
  if (pulseLength >= 1480 && pulseLength <= 1540)
  {
      currentState = 1;
      prevState = 1;
  }
}

boolean ThreeStateDoubleSwitch::isUpperSwitchOn() {
  return upperSwitchOn;
}

boolean ThreeStateDoubleSwitch::isLowerSwitchOn() {
  return lowerSwitchOn;
}
