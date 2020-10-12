
#include "sbus.h"
/*
 * SBUS part
 */

sbusChannels_t sbus;

void sbus_start() 
{
  Serial.begin(100000,SERIAL_8E2);
  memset(&sbus,0,sizeof(sbus));
}

void sbus_send() {
  Serial.write("\x0F",1);
  Serial.write((uint8_t*)&sbus,sizeof(sbus));
  Serial.write("\x00",1);
}
