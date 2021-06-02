#include <Arduino.h>
#include "inp_x.h"

static volatile uint8_t adc_chan, adc_next, adc_prev;
static volatile uint16_t adc_values[ADC_SIZE];

#define ADC_CONFIG  _BV(ADEN) | _BV(ADSC) | _BV(ADATE) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0)
#define ADC_REFSEL  _BV(REFS0) /*| _BV(REFS1)*/

#define CHAN_NEXT(n) (((n)>=(ADC_SIZE-1))?0:(n)+1)
#define CHAN_PREV(n) (((n)==0)?ADC_SIZE-1:(n)-1)

uint8_t adc_chan_mux(uint8_t n)
{
  //return (((n)&4)?((n)==4)?0x1F:0x1E:7-(n));
  //return (n&1)?0x01:0x1E;
  switch(n) {
  case 0: return 0x07;  case 1: return 0x06;  case 2: return 0x05;  case 3: return 0x04;  case 4: return 0x01;  case 5: case 6: case 7: return 0x1E;
//  case 0: return 0x06;  case 1: return 0x06;  case 2: return 0x06;  case 3: return 0x06;  case 4: return 0x06;  case 5: return 0x06;
//  case 0: return 0x1e;  case 1: return 0x1e;  case 2: return 0x1e;  case 3: return 0x1e;  case 4: return 0x1e;  case 5: return 0x1e;
  }
}

ISR(ADC_vect)
{
#if defined(__AVR_ATmega32U4__)
/*
 * 0x07 ADC7 = A0 0
 * 0x06 ADC6 = A1 1
 * 0x05 ADC5 = A2 2
 * 0x04 ADC4 = A3 3
 * 0x01 ADC1 = A4 4
 * 0x00 ADC0 = A5
 * 0x1E BANDGAP   5
 * 0x1F GND
 */
  adc_values[adc_prev] = ADC;
  ADMUX = adc_next;
  adc_prev = adc_chan;
  adc_chan = CHAN_NEXT(adc_chan);
  adc_next = adc_chan_mux(CHAN_NEXT(adc_chan)) | ADC_REFSEL;
#endif
}

uint16_t adc_read(uint8_t chan)
{
  uint16_t v;
  noInterrupts();
  v = adc_values[chan];
  interrupts();
  return v;
}

void adc_setup()
{
  // 125khz (ADPS=0b111, 16MHz div128)
  noInterrupts();
  adc_prev = 0;
  adc_chan = 0;
  adc_next = adc_chan_mux(CHAN_NEXT(adc_chan)) | ADC_REFSEL;
  ADMUX  = adc_chan_mux(adc_chan);
  ADCSRA = ADC_CONFIG;
  ADCSRB = _BV(ADHSM);
  DIDR0  = 0xF0; // digital input disable channel 4..7
  interrupts();
}
