#include <SPI.h>
#include "yd717_protocol.h"

nrf24l01p wireless; 
yd717Protocol protocol;

#define pinLED 18

uint32_t tmrLED;
uint32_t tmrLED_wait;

byte tmrLED_chk(uint32_t t) // call with micros(), millis()
{
    if (tmrLED_wait==0xFFFFFFFF) { tmrLED = t; return 0; }
    if (tmrLED_wait==0) { tmrLED = t; return 1; }
    if (t - tmrLED < tmrLED_wait)  return 0;
    tmrLED = t; return 1;
}
void tmrLED_set(uint32_t dt) { tmrLED_wait=dt; }

void tmrLED_start(uint32_t now) { tmrLED = now; } // call with micros(),millis();
void tmrLED_stop() { tmrLED_set(0xFFFFFFFF); }

unsigned long time = 0;
 
void setup() {
  // SS pin must be set as output to set SPI to master !
  pinMode(SS, OUTPUT);
  pinMode(pinLED, OUTPUT);
  Serial.begin(115200);
  // CE & nCS pin to D8, D7
  wireless.setPins(9, 10);
  protocol.init(&wireless);
  
  Serial.println("Start");
  tmrLED_set(333);
  tmrLED_start(micros());
}

rx_values_t rxValues;
bool bind_in_progress = true;

void loop() 
{
  switch(protocol.run(&rxValues)) {
  case  BIND_IN_PROGRESS:
      if(!bind_in_progress) {
        bind_in_progress = true;
        Serial.println("Bind in progress");
      }
  break;
  case BOUND_NEW_VALUES:
      digitalWrite(pinLED, 1);
      Serial.print(" :\t");Serial.print(rxValues.throttle);
      Serial.print("\t"); Serial.print(rxValues.yaw);
      Serial.print("\t"); Serial.print(rxValues.pitch);
      Serial.print("\t"); Serial.print(rxValues.roll);
      Serial.print("\t"); Serial.print(rxValues.trim_yaw);
      Serial.print("\t"); Serial.print(rxValues.trim_pitch);
      Serial.print("\t"); Serial.print(rxValues.trim_roll);
      Serial.print("\t"); Serial.println(rxValues.flags);
  break;
  case NOT_BOUND:
      if (tmrLED_chk(millis())) { digitalWrite(pinLED,1-digitalRead(pinLED)); }
  break;
}
  delay(2); // ? 2 ms?
}

