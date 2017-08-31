/*
 * AD7714 Library
 * sample code
 */

#include <AD7714.h>

AD7714 adc(2.5);
double v;

void setup()
{
  Serial.begin(9600);

  adc.reset();
  adc.init(AD7714::CHN_1);  
  adc.init(AD7714::CHN_2);
}

void loop()
{
  v = adc.read(AD7714::CHN_1);
  Serial.print(v);

  v = adc.read(AD7714::CHN_2);
  Serial.print(" : ");
  Serial.println(v);
}
