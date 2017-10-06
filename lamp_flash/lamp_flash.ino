#include "PinChangeInt.h"

enum { stb = 0, zcd = 4 };

volatile int  cross;
volatile int  angle;
volatile uint32_t  ft;

void zerocross()
{ 
  cross=1;
}

void setup()
{
  pinMode(stb, OUTPUT);
  digitalWrite(stb, HIGH);
  pinMode(zcd, INPUT);
  cross=0;
  angle=0;
  ft=0;
  attachPinChangeInterrupt(zcd, zerocross, CHANGE);
}


void loop()
{
  uint32_t t;
  int32_t d;
  t = micros();
  if (cross) { cross=0; ft=t+angle; }
  d = t-ft;
  if (d >= 0) { digitalWrite(stb, LOW);
          delayMicroseconds(100);
        digitalWrite(stb, HIGH);
        ft += 10000;
  }
}
