#include <avr/io.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include "config.h"
#include "fncs.h"

//#include "config.h"

#define TEMP025 599
#define TEMP290 630
/*
void terminal()
{
	uint8_t k;
	char buf[8];

	for(;;) {
		if ((k=scan_key()) && (k&0x1F) != 0x16)
				ser_put(keymap(k));
		if ((k&0x1F) == KEY_STOP) {
			k = lcd_hw_read(0); // get AC
			sprintf(buf, "lcd%02X\r\n",k);
			ser_send(buf,7);
		}	
		if (ser_available())
			lcd_hw_write(LCD_FL_DATA, cp1251lcd(ser_get()));
	}
}
*/

static uint32_t t0;
//static uint16_t y;
/*
void terminal()
{
	uint32_t t;
	for(;;) {
	        t=ticks();
		if (t-t0 >= USEC2TICK(10000L)) t0=t;
		else continue;
//		if (++y>=10) { y=0; ser_printInt(t0); ser_println(); }
		if (ser_available())
			ser_write(ser_read());

		uint16_t x = adc_get(); if (x == 0xFFFF) continue;
		ser_printInt(x);
		ser_println();

	}
}
*/

//void terminal() { for(;;) if (SPIN & SRXBIT) SPORT |=STXBIT; else SPORT &= ~STXBIT; }
extern uint16_t _rx_delay_stopbit;
void loop()
{
	uint16_t x;
	for(;;) {
/*
		if (!ser_available()) continue;
		x=ser_read();
		ser_write(x);
		ser_printInt(_rx_delay_stopbit);
*/
		_delay_ms(500);
		x=adc_get();
		ser_printInt(x); ser_println();
//		switch(x) {
//		case ' ': x=adc_get(); ser_printInt(_rx_delay_stopbit); ser_println(); break;
//		case 'p': x=ser_parseInt(); ser_print("pwm:"); ser_printInt(x); ser_println(); timer_pwm(x); break;
//		}
	}
}


int main(void)
{
//	t0=0;
	timer_init(TMODE);
//	timer_pwm(0);
	ser_init(SBAUD);
	adc_init(AMODE);
//	terminal();
	loop();
	return 0;
}

