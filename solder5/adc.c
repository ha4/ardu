#include <avr/io.h>
#include <avr/interrupt.h>
#include "config.h"
#include "fncs.h"

volatile uint16_t _adc_val;

void adc_init(uint8_t amode)
{
	_adc_val=0xFFFF;
	ADCSRB = 0; // analog comp multiplexer enable, AIN1(-)=D7 unused
	ADMUX = AREFERENCE | ACHANNEL;
 	ADCSRA = (1<<ADEN) | amode | (0x6 <<ADPS0); // enable ADC, prescaler div64
	ACSR |= (1<<ACD)|(0<<ACBG);  // disable ac, no bangap
}

ISR(ADC_vect)
{
	uint8_t low, high;
	low  = ADCL;
	high = ADCH;
	_adc_val= (high << 8) | low;
}

uint16_t adc_get()
{
	uint16_t v;
	uint8_t oldSREG = SREG;
	cli();
	v = _adc_val;
	_adc_val=0xFFFF;
	SREG = oldSREG;
	return v;
}

/*
uint16_t adc_read()
{
	uint8_t low, high;
        uint16_t res;
	ADCSRA|= _BV(ADSC); // start the conversion
	while(ADCSRA&_BV(ADSC)); // ADSC is cleared when the conversion finishes
	low  = ADCL;
	high = ADCH;
	res = (high << 8) | low;
        return res;

}
*/
