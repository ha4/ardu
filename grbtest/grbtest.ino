#include <GRBlib.h>

rgbled led(-1,-1,-1);

uint8_t h,s,l;

void setup()
{
  Serial.begin(9600);
  h=0;
}

void loop()
{
  if (!Serial.available()) return;
  h = Serial.parseInt();
  s = Serial.parseInt();
  l = Serial.parseInt();
  led.hsl(h,s,l);
}

