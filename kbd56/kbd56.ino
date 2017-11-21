#include <stdint.h> 
#include <stdbool.h>
#include "config.h"
#include "keycode_set2.h"
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

static unsigned char last_send=0;

unsigned char sendcode(unsigned char c)
{
  if (c < 16) Serial.print('0'); else Serial.print(c>>4,HEX);
  Serial.print(c&15,HEX);
  if (!ps2_txClear()) ps2_host_protocol();
  if (!ps2_txSet(c))
      return ps2_host_protocol();
  last_send = c;
  return PS2_NORMAL;
}

static unsigned char layer = 0;

void action_exec(keyevent_t e)
{
    unsigned char buf[16], n, rn, l;
    byte scancode;
    if (IS_NOEVENT(e)) { return; }
    n=0;
    l=layer;
    do 
      scancode =  keymap_get_keycode(l,e.key.row,e.key.col);
    while (l-- && scancode == KC_TRANSPARENT);
    if (scancode == KC_TRANSPARENT) return;
    
    if (scancode == KC_FN0) {
        if (e.pressed) layer = 1;
        else layer = 0;
    }

    if ((scancode & 0x80) && scancode!=0x83) {
      buf[n++]=0xE0;
      scancode &= ~0x80;
    }
    if (!e.pressed) buf[n++]=0xF0;
    buf[n++]=scancode;
    rn = 5; // 5 times repeat
    while(rn--) {
      for(unsigned char x=0; x < n; x++) {
        if (sendcode(buf[x])==PS2_REPEAT) goto REP;
      }
      break;
      REP:;
    }
}

void setup()
{
  Serial.begin(115200);
  keyboard_setup();
  keyboard_init();
  ps2_init();
  ps2_reset();
//  timer2_init();
          Serial.print("INIT COMPLETE");

}

void loop()
{
  keyboard_task();
  switch(ps2_host_protocol()) {
  case PS2_RESET: keyboard_init(); break;
  case PS2_REPEAT: ps2_txSet(last_send); break;
  default: break;
  }
}


