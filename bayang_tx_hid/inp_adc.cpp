#include <Arduino.h>
#include "inp_x.h"

static uint8_t adc_chan, adc_next;
static volatile uint16_t adc_values[4];


ISR(ADC_vect)
{
#if defined(__AVR_ATmega32U4__)
  adc_values[adc_chan & 0x3] = ADC;
  ADMUX = adc_next;
  adc_chan = adc_next;
  adc_next = ((adc_next + 1) & 0x3) | 0x04 | _BV(REFS0);
#endif
}

uint16_t adc_read(uint8_t chan)
{
  uint16_t v;
  noInterrupts();
  v = adc_values[((~chan) + 1) & 0x3];
  interrupts();
  return v;
}

void adc_setup()
{
  // 125khz (ADPS=0b111, 16MHz div128)
  noInterrupts();
  adc_chan = adc_next = 0x04 | _BV(REFS0);
  ADMUX  = adc_chan;
  ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADATE) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
  ADCSRB = 0;
  DIDR0  = 0xF0; // digital input disable channel 4..7
  interrupts();
}
