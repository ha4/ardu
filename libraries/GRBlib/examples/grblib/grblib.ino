#include <GRBlib.h>
// pwm pins: 3 5 6 9 10 11

rgbpwm L1(3,5,6);

int i;

void setup()
{
    i=0;
}

void loop()
{
  L1.hsv(i,100,100);
  i++;
  delay(20);
}

