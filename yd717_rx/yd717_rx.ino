#include <SPI.h>
#include "yd717_protocol.h"

nrf24l01p wireless; 
yd717Protocol protocol;

unsigned long time = 0;
 
void setup() {
  // SS pin must be set as output to set SPI to master !
  pinMode(SS, OUTPUT);
  Serial.begin(115200);
  // CE & nCS pin to D8, D7
  wireless.setPins(9, 10);
  protocol.init(&wireless);
  
  time = micros();
  Serial.println("Start");
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
      Serial.print(" :\t");Serial.print(rxValues.throttle);
      Serial.print("\t"); Serial.print(rxValues.yaw);
      Serial.print("\t"); Serial.print(rxValues.pitch);
      Serial.print("\t"); Serial.print(rxValues.roll);
      Serial.print("\t"); Serial.print(rxValues.trim_yaw);
      Serial.print("\t"); Serial.print(rxValues.trim_pitch);
      Serial.print("\t"); Serial.print(rxValues.trim_roll);
      Serial.print("\t"); Serial.println(rxValues.flags);
  break;
  }
  delay(2); // ? 2 ms?
}

