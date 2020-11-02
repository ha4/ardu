#include <Arduino.h>
#include "ppm.h"
/* 
 PPM CONFIGURATION
*/
#define PPM_pin 2  //set PPM signal output pin on the arduino
#define PPM_FrLen 27000  //set the PPM frame length in microseconds (1ms = 1000µs)
#define PPM_PulseLen 400  //set the pulse length
#define PPM_on PORTD |= _BV(2)
#define PPM_off PORTD &= ~_BV(2) 

int ppm[PPM_number];

void setupPPM() {
  pinMode(PPM_pin, OUTPUT);
  PPM_off;

  cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;

  OCR1A = 100;  // compare match register (not very important, sets the timeout for the first interrupt)
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei();
}

//#error This line is here to intentionally cause a compile error. Please make sure you set clockMultiplier below as appropriate, then delete this line.
#define clockMultiplier 2 // set this to 2 if you are using a 16MHz arduino, leave as 1 for an 8MHz arduino

ISR(TIMER1_COMPA_vect){
  static boolean pulse = true;
  static byte n=0;
  static unsigned int rest=PPM_FrLen;

  TCNT1 = 0;

  if(pulse) { //end pulse
    PPM_off;
    OCR1A = PPM_PulseLen * clockMultiplier;
    pulse = false;
    return;
  }

  PPM_on;
  pulse = true;

  if(n >= PPM_number) {
      rest -= PPM_PulseLen;
      OCR1A = rest * clockMultiplier;
      rest = PPM_FrLen;
      n=0;
  } else {
      rest -= ppm[n];
      OCR1A = (ppm[n] - PPM_PulseLen) * clockMultiplier;
      n++;
  }     
}
