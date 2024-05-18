#include <Arduino.h>
#include "PushPullButton.h"

#define DEBUG

#ifdef DEBUG
#define DEBUG_MSG_ONELINE(x) Serial.print(F(x))
#define DEBUG_MSG(x) Serial.println(F(x))
#define DEBUG_MSG_VAR(x) Serial.println(x)
#else
#define DEBUG_MSG(x) // define empty, so macro does nothing
#define DEBUG_MSG_VAR(x)
#define DEBUG_MSG_ONELINE(x)
#endif

PushPullButton::PushPullButton(int pin, unsigned long upTime) :
  DigitalOutput(pin),
  mUpTime(upTime)
{ }

void PushPullButton::Enable() {
  DigitalOutput::Enable();
  mStartTime = millis();
  DEBUG_MSG("PushPullButton::Enable");
}

void PushPullButton::Handle() {
  if(eActive == mState
    && ((millis() - mStartTime) > mUpTime)) {
      DigitalOutput::Disable();
      mStartTime = 0;
      DEBUG_MSG("PushPullButton::Disable");
  }
}
