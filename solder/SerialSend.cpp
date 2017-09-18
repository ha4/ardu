#include <Arduino.h>
#include "SerialSend.h"

/* static */ 
inline void SerialSend::tunedDelay(uint16_t delay) { 
  uint8_t tmp=0;
  asm volatile("sbiw    %0, 0x01 \n\t"
    "ldi %1, 0xFF \n\t"
    "cpi %A0, 0xFF \n\t"
    "cpc %B0, %1 \n\t"
    "brne .-10 \n\t"
    : "+r" (delay), "+a" (tmp)
    : "0" (delay)
    );
}

#if F_CPU == 16000000
#define XMIT_DELAY 233
#define XMIT_START_ADJUSTMENT 5
#elif F_CPU == 8000000
#define XMIT_DELAY 112
#define XMIT_START_ADJUSTMENT 4
#elif F_CPU == 20000000
#define XMIT_DELAY  294
#define XMIT_START_ADJUSTMENT 6
#else
#error This version of SoftwareSerial supports only 20, 16 and 8MHz processors
#endif

void SerialSend::begin(long speed)
{
  pinMode(pin,OUTPUT);
  digitalWrite(pin, 1);
  _tx_delay = XMIT_DELAY;
}


size_t SerialSend::write(uint8_t b)
{
  uint8_t oldSREG = SREG;
  cli();  // turn off interrupts for a clean txmit

  digitalWrite(pin,0);
  tunedDelay(_tx_delay + XMIT_START_ADJUSTMENT);
  for (byte mask = 0x01; mask; mask <<= 1) {
    if (b & mask) digitalWrite(pin,1);
       else       digitalWrite(pin,0);
    tunedDelay(_tx_delay);
  }
  digitalWrite(pin,1);
  SREG = oldSREG; // turn interrupts back on
  tunedDelay(_tx_delay);

  return 1;
}


