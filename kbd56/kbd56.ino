#include <stdint.h>
#include <stdbool.h>
#include "keyboard.h"
#include "ps2device.h"

#define COUNT_UP    (256 - 33)

void timer2_init()
{
    TCCR2A=0;
    TCCR2B=(0b010<<CS20);// clk/div8, mode0
    TCNT2=COUNT_UP;  /* value counts up from this to zero */
    TIMSK2 |= _BV(TOIE2);  // enable TCNT2 overflow interrupt
}

ISR(TIMER2_OVF_vect)
{
  TCNT2 = COUNT_UP;
  ps2_process();
}


void setup()
{
  Serial.begin(115200);
  keyboard_setup();
  keyboard_init();
  timer2_init();
          Serial.print("INIT COMPLETE");

}

void loop()
{
  keyboard_task();
  if (ps2_rxAvailable())
    Serial.println(ps2_rxGet(),HEX);
}



    /* restart timer */
//    outp(COUNT_UP, TCNT0);  /* value counts up from this to zero */
    
