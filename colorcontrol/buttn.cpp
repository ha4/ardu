#if defined(ARDUINO) && (ARDUINO >= 100)
#	include <Arduino.h>
#else
#	if !defined(IRPRONTO)
#		include <WProgram.h>
#	endif
#endif

#include "buttn.h"

button::button(int pin)
{
  p=pin;
  stat=BTN_NONE;
  prev=0; cnt=0; flt = 0; cc=0;
  pinMode(p, INPUT_PULLUP);
  for(int j=0; j < 8; j++) debounce();
}

#define TICKS_DC (BTN_DC/BTN_PERIOD)
#define TICKS_LC (BTN_LC/BTN_PERIOD)

byte button::poll()
{
  byte i = debounce();
  if (i == prev) {
    if (cnt != 255) cnt++;
    if (i==0 && cnt > TICKS_DC && stat != BTN_NONE) {
        if (stat == BTN_MCLICK && cc==1) i = BTN_DCLICK; else i=stat;
        stat = BTN_NONE;
        cc = 0;
        return i;
    }
    return BTN_NONE;
  }
  prev = i;
  if (i) {
    if (cnt <= TICKS_DC)  { stat = BTN_MCLICK; cc++; }  else stat=BTN_CLICK;
    cnt = 0;
    return BTN_PRESS;
  } else {
    if (cnt >= TICKS_LC) stat = BTN_LONG; 
    cnt = 0;
    return BTN_RELEASE;
  }
}

byte button::debounce()
{
  byte i = digitalRead(p);

  // debounce y=0.75y+0.25x: y=y-0.25y+0.25x: y+=0.25x-0.25y: 8y=2x-2y
  flt += i+i - (flt>>2);
  if (flt > 4) return 0; else return 1;
}


