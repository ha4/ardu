#include "PinChangeInt.h"

#define RC_PIN_CH0 17
#define RC_PIN_PLUS 16
#define RC_PIN_GND 15

int PWM_rx, PWM_update;

inline void decodePWM(uint32_t microsIsrEnter, bool risingEdge)
{
}


void intDecodePWM()
{ 
  static uint32_t microsRisingEdge;
  uint16_t pulseInPWM;

  if (PCintPort::pinState==HIGH)  {
    microsRisingEdge = PCintPort::microsIsrEnter;
    return;
  }
  pulseInPWM = PCintPort::microsIsrEnter - microsRisingEdge;
  if ((pulseInPWM >= 900) && (pulseInPWM <= 2100)) {
      pulseInPWM = 16*PWM_rx + (pulseInPWM - PWM_rx);
      PWM_rx = pulseInPWM / 16;
      PWM_update=true;
  }
}

void setup()
{
    pinMode(RC_PIN_CH0, INPUT);
    pinMode(RC_PIN_GND, OUTPUT);
    digitalWrite(RC_PIN_GND, 0);
    pinMode(RC_PIN_PLUS, OUTPUT);
    digitalWrite(RC_PIN_PLUS, 1);
    PCintPort::attachInterrupt(RC_PIN_CH0, &intDecodePWM, CHANGE);
    Serial.begin(57600);
}

void loop()
{
  if (PWM_update) {
    PWM_update=0;
    Serial.println(PWM_rx);
  }
}

