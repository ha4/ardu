
#include <Arduino.h>
#include "buttn.h"

unsigned long btc=0;

button b1(4);

void setup()
{
  pinMode(2, OUTPUT);
  digitalWrite(2, 1);
  pinMode(13, OUTPUT);
  digitalWrite(13, 1);

  for(int i=10; i--; delay(BTN_PERIOD)) b1.debounce();
  while(b1.debounce()==1); // wait release
  digitalWrite(13, 0);
  delay(100);
  digitalWrite(13, 1);
}

void flashes(int n)
{
  for(;n--;) {
    digitalWrite(13,0);
    delay(250);
    digitalWrite(13,1);
    delay(250);
  }
}

void powerdown()
{
  digitalWrite(2, 0);
  for(;;); 
}


void loop()
{
  unsigned long t=millis();
  if (t-btc >= BTN_PERIOD) {btc=t; switch(b1.poll()) {
    case BTN_CLICK: flashes(1); break;
    case BTN_DCLICK: flashes(2); break;
    case BTN_MCLICK: flashes(3); break;
    case BTN_LONG: flashes(5); 
    powerdown(); break;
  }}
}

