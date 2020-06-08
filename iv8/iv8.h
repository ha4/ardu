#ifndef __IV8_H

#include "Arduino.h"

#define USE_SERIAL

// pwm 3,5,6,9,10,11
#define UREF 1096
#define U1_PIN 2//A6
#define U1_PWM 3
#define U2_PIN 1///A7
#define U2_PWM 5
extern void ustab1();
extern void ustab2();
extern void ushow();

void i_serial(char c);

#endif

