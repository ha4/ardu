#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "keyboard.h"
#include "ps2device.h"
#include "matrix.h"

#define COUNT_UP    (256 - 33)

void timer2_init()
{
    TCCR2A=0;
    TCCR2B=(0b010<<CS20);// clk/div8, mode0
    TCNT2=COUNT_UP;  /* value counts up from this to zero */
    TIMSK2 |= _BV(TOIE2);  // enable TCNT2 overflow interrupt
}

//ISR(TIMER2_OVF_vect)
//{
//  TCNT2 = COUNT_UP;
//  ps2_process();
//}
uint8_t keymap_get_keycode(uint8_t layer, uint8_t row, uint8_t col);

void sendcode(unsigned char c)
{
  if (c < 16) Serial.print('0'); else Serial.print(c>>4,HEX);
  Serial.print(c&15,HEX);
  if(ps2_txClear()) if (!ps2_txSet(c)) Serial.print('!');
}

void action_exec(keyevent_t e)
{
    if (IS_NOEVENT(e)) { return; }
    byte scancode =  keymap_get_keycode(0,e.key.row,e.key.col);
    if (scancode & 0x80) {
      sendcode(0xE0);
      scancode &= ~0x80;
    }
    if (!e.pressed) sendcode(0xF0);
    sendcode(scancode);
   Serial.println();
}

void setup()
{
  Serial.begin(115200);
  keyboard_setup();
  keyboard_init();
  ps2_init();
//  timer2_init();
          Serial.print("INIT COMPLETE");

}

void loop()
{
	unsigned char v;
  keyboard_task();
  if (ps2_rxAvailable() && ps2_rxGet(&v))
    Serial.println(v,HEX);
}



    /* restart timer */
//    outp(COUNT_UP, TCNT0);  /* value counts up from this to zero */
    
