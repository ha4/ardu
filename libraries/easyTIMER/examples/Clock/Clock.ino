#include <easyTIMER.h>

enum { ledPin = 13 };

easyTIMER t1(CLK_OSC_DIV256, CLK_M4_CTC, CLK_IRQ_MATCHA);

ISR(TIMER1_COMPA_vect)
{
  digitalWrite(ledPin, digitalRead(ledPin) ^ 1);   // toggle LED pin
}

void setup()
{
   pinMode(ledPin, OUTPUT);
   
   t1.clk_start();
   t1.clk_seta(31249); // 16Mhz/2Hz = 8e6; 8e6/256 = 31250
}

void loop()
{
}
