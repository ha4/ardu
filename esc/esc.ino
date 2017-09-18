/* pwm-ENABLED 3,   5,   6,   9,  10,  11  5,6-980Hz, other 490Hz
             pd3, pd5, pd6, pb1, pb2, pb3
            oc2b,oc0b,oc0a,oc1a,oc1b,oc2a
MODE: +PWM(act=0) -ON(act=1)
*/  
#include "PinChangeInt.h"

enum { 
  RC_in = 10,
  motorPinAH=3,
  motorPinBH=18,
  motorPinCH=5,
  motorPinAL=4,
  motorPinBL=19,
  motorPinCL=7,
  motorPinSync=13,
  motorACO=12,
  motorCOMP=6, // FIXED PIN, AIN0-comp, middle "Y"
  _phA = 0,    // AIN1 - muxed ADC0=PC0 -> A0
  _phB = 1,    //muxed ADC1=PC1 -> A1
  _phC = 2,    //muxed ADC0=PC0 -> A0
};

/* deactivate/activate levels for high/low bridge transistor */
#define ACT_H 0
#define DEA_H 1
#define ACT_L 1
#define DEA_L 0

#define HI_EDGE  (0<<ACO)
#define LOW_EDGE (1<<ACO)

uint32_t throttle;
#define SPEED 1800

int PWM_rx, PWM_update;

void intDecodePWM()
{ 
  static uint32_t microsRisingEdge;
  uint16_t pulseInPWM;

  if (PCintPort::pinState==HIGH)  {
    microsRisingEdge = PCintPort::microsIsrEnter;
    return;
  }
  pulseInPWM = PCintPort::microsIsrEnter - microsRisingEdge;
  if ((pulseInPWM >= 900) && (pulseInPWM <= 2100)) {
      pulseInPWM = 16*PWM_rx + (pulseInPWM - PWM_rx);
      PWM_rx = pulseInPWM / 16;
      PWM_update=true;
  }
}



void setup() {
  Serial.begin(57600);
  throttle = 2500;
//pins
  pinMode(motorPinAL, OUTPUT);
  digitalWrite(motorPinAL, DEA_L);
  pinMode(motorPinAH, OUTPUT);
  digitalWrite(motorPinAH, DEA_H);
  pinMode(motorPinBL, OUTPUT);
  digitalWrite(motorPinBL, DEA_L);
  pinMode(motorPinBH, OUTPUT);
  digitalWrite(motorPinBH, DEA_H);
  pinMode(motorPinCL, OUTPUT);
  digitalWrite(motorPinCL, DEA_L);
  pinMode(motorPinCH, OUTPUT);
  digitalWrite(motorPinCH, DEA_H);
  pinMode(motorPinSync, OUTPUT);
  digitalWrite(motorPinSync, 0);
  pinMode(motorACO, OUTPUT);
  digitalWrite(motorACO, 0);

// timers
//  TCCR2=0;     // pwm, stopped
//  TCCR1B=(1<<CS10);// commutation, RCpulse
// T0: beep, delays

  delay(30); // settle
  // comp_init
  ADCSRB|= 1<<ACME; // multiplex enable
  ADCSRA&=~(1<<ADEN); // disable ADC
  ACSR  &=~(1<<ACD);  // enable COMP

// RC stuff  
  pinMode(RC_in, INPUT);
  PCintPort::attachInterrupt(RC_in, &intDecodePWM, CHANGE);

}

#define wait_for(e)  while((ACSR ^ e) & (1 << ACO)); delayMicroseconds(2000-PWM_rx);

void loop()
{
uint32_t y, x = micros();
ADMUX=_phA;
digitalWrite(motorPinBH, ACT_H);
digitalWrite(motorPinCL, ACT_L);

for(;;){
  wait_for(HI_EDGE);    // ... a==hi ->

//  com1com2 B+:off A+:on     +AC-
  ADMUX=_phB;
  digitalWrite(motorPinBH, DEA_H);
  digitalWrite(motorPinAH, ACT_H);
  digitalWrite(motorPinSync, 0); //  sync_off
  wait_for(LOW_EDGE);  // ... b==low ->

//  com2com3 C-:off B-:on     +AB-
  ADMUX=_phC;
  digitalWrite(motorPinCL, DEA_L);
  digitalWrite(motorPinBL, ACT_L);
  wait_for(HI_EDGE);   // ... c==hi ->

//  com3com4 A+:off C+:on     +CB-
  ADMUX=_phA;
  digitalWrite(motorPinAH, DEA_H);
  digitalWrite(motorPinCH, ACT_H);
  wait_for(LOW_EDGE);  // ... a==low ->

//  com4com5 B-:off A-:on     +CA-
  ADMUX=_phB;
  digitalWrite(motorPinBL, DEA_L);
  digitalWrite(motorPinAL, ACT_L);
  digitalWrite(motorPinSync, 1); //  sync_on
  wait_for(HI_EDGE);   // ... b==hi ->

//  com5com6 C+:off B+:on     +BA-
  ADMUX=_phC;
  digitalWrite(motorPinCH, DEA_H);
  digitalWrite(motorPinBH, ACT_H);
  wait_for(LOW_EDGE);  // ... c==low ->

//  com6com1 A-:off C-:on     +BC-
  ADMUX=_phA;
  digitalWrite(motorPinAL, DEA_L);
  digitalWrite(motorPinCL, ACT_L);

// stuff:   y=micros();  if (y-x > 3000) {  x=y; if (throttle != SPEED) throttle -= 10;}
//  if (PWM_update) {    PWM_update=0;    Serial.println(PWM_rx);  }
}
}





