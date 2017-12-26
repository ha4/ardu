#include <Arduino.h>
#include "phase.h"
#include "PinChangeInt.h"

/*
    +5v ---[4.7k]-------------.  
  pulse ----------------------+
            ,--------[15k]----|--------------------.
            `-#1        #4----'                    |
            .-#2 PC814  #3--GND                    |
            `----------[15k]------------------.    |
                                              |    |
   out -[220]-#1        #6----------T1--------+----|---o L
   gnd -------#2 MOC052 #5          T2------HEATER-+---o N
              #3        #4--[1.5k]--Gate
*/

#define DETECT 14  //zero cross detect
//#define DETECT 2  //zero cross detect
#define GATE 15    //TRIAC gate

#define PULSE 4   //trigger pulse width (counts)
void zeroCrossingInterrupt();
static volatile int fangle;

void init_acphase()
{

  // set up pins
  pinMode(DETECT, INPUT);     //zero cross detect
  digitalWrite(DETECT, 1); //enable pull-up resistor
  digitalWrite(GATE, 0);
  pinMode(GATE, OUTPUT);      //TRIAC gate control

  // set up Timer1 
  //(see ATMEGA 328 data sheet pg 134 for more details)
  set_acphase(545);
  noInterrupts();
  TIMSK1 = _BV(TOIE1)|_BV(OCIE1B);    //enable comparator B and overflow interrupts
  TCCR1A = 0x00;    //timer control registers set for
  TCCR1B = 0x00;    //normal operation, timer disabled
  TCNT1 = 1;

  TIFR1=0xff;
  interrupts();
  // set up zero crossing interrupt
  attachPinChangeInterrupt(DETECT,zeroCrossingInterrupt, FALLING);  
//    attachInterrupt(0,zeroCrossingInterrupt, FALLING);  

}  


void zeroCrossingInterrupt()
{
  digitalWrite(GATE, 0); //turn off TRIAC gate
  TCCR1B=_BV(CS12); //start timer with divide by 256 input; 16us
  TCNT1 = 0;   //reset timer - count from zero
  OCR1B = fangle;
}

ISR(TIMER1_COMPB_vect)
{
  digitalWrite(GATE, 1);  //set TRIAC gate to high
  TCNT1 = 65536-PULSE;      //trigger pulse width
}

ISR(TIMER1_OVF_vect)
{
  digitalWrite(GATE, 0); //turn off TRIAC gate
  TCCR1B = 0x00;          //disable timer stopd unintended triggers
}

void set_acphase(int i)
{
   fangle=i;
}

