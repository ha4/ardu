#include <avr/io.h>
#include <avr/interrupt.h>
#include "config.h"
#include "fncs.h"

volatile unsigned long timer0_overflow_count;

ISR(TIM0_OVF_vect)
{
	timer0_overflow_count++;
}

ISR(TIM0_COMPB_vect)
{
	adc_start();
}


unsigned long ticks() {
	unsigned long m;
	uint8_t oldSREG = SREG, t;
	
	cli();
	m = timer0_overflow_count;
	t = TCNT0;

	if ((TIFR0 & _BV(TOV0)) && (t < 255))
		m++;

	SREG = oldSREG;
	
	return ((m << 8) + t); //micros() * (TODIV * 1000000L / F_CPU);
}

void timer_init(uint8_t tmode)
{
	sei();
	timer0_overflow_count = 0;
	TCCR0A |= _BV(WGM01) | _BV(WGM00);
	TCCR0B |= (TCLKSEL)<<CS00; // clk/256
	TIMSK0 |= _BV(TOIE0) | tmode;
}

void timer_pwm(uint8_t x)
{
	DDRB |= PWMBIT;
	if (x == 0) {
		TCCR0A &= ~(_BV(COM0A1)|_BV(COM0A0));
		PORTB |= PWMBIT;
	}
	else
	{
		TCCR0A |= _BV(COM0A1)|_BV(COM0A0);
	}
	OCR0A = x; // set pwm duty
	OCR0B = x+12;
}
