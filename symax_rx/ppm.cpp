#include <Arduino.h>
#include "ppm.h"

uint16_t ppm[PPM_number];

uint8_t PPM_start;
uint8_t PPM_chan;
uint16_t PPM_rest;

void ppm_start()
{
  for(int i=0; i<PPM_number; i++)
    ppm[i]= default_servo_value;

  pinMode(pinPPM, OUTPUT);
  pinMode(pinSync, OUTPUT);
  digitalWrite(pinPPM, !PPM_onState);  //set the PPM signal pin to the default state (off)
  
  cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 100;  // compare match register, change this
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  PPM_start = 1;
  PPM_chan = 0;
  PPM_rest = PPM_FrLen;
  sei();
}

void ppm_stop()
{
  cli();
  TCCR1A = 0;
  TCCR1B = 0; // no timer clocking
  TIMSK1 &= ~(1 << OCIE1A);
  sei();
  digitalWrite(pinPPM, !PPM_onState);
}

ISR(TIMER1_COMPA_vect)
{
  TCNT1 = 0;
  
  if (PPM_start) {  //start pulse
    digitalWrite(pinPPM, PPM_onState);
    OCR1A = PPM_PulseLen * PPM_micros;
    PPM_start = 0;
    return;
  }
  digitalWrite(pinPPM, !PPM_onState);

  if(PPM_chan < PPM_number){
    OCR1A = (ppm[PPM_chan] - PPM_PulseLen) * PPM_micros;
    PPM_rest -= ppm[PPM_chan];
    PPM_chan++;
  } else {
    OCR1A = PPM_rest * PPM_micros;
    PPM_rest = PPM_FrLen;
    PPM_chan = 0;  
  }
  PPM_start = 1;
}
