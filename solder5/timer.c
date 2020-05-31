#include <avr/io.h>
#include <avr/interrupt.h>
#include "config.h"
#include "fncs.h"

#define TODIV 1024
#define clockCyclesPerMicrosecond() ( F_CPU / 1000000L )
#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(TODIV * 256))
volatile unsigned long timer0_overflow_count = 0;

ISR(TIMER0_OVF_vect)
{
	timer0_overflow_count++;
}

ISR(TIMER0_COMPB_vect)
{
	adc_start();
}

unsigned long micros() {
	unsigned long m;
	uint8_t oldSREG = SREG, t;
	
	cli();
	m = timer0_overflow_count;
	t = TCNT0;

	if ((TIFR & _BV(TOV0)) && (t < 255))
		m++;

	SREG = oldSREG;
	
	return ((m << 8) + t) * (TODIV / clockCyclesPerMicrosecond());
}

void timer_init(uint8_t tmode)
{
	sei();
	
	TCCR0A |= _BV(WGM01) | _BV(WGM00);
	TCCR0B |= _BV(CS02) | _BV(CS00); // clk/1024
	TIMSK |= _BV(TOIE0) | tmode;
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
