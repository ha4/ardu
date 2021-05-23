#ifndef __PPM_ISR_H_
#define __PPM_ISR_H_

enum { pinPPM = 5, pinSync= 4} ;

#define PPM_number 8  //set the number of chanels
#define PPM_micros 2  // timer resolution scale factor 16Mhz==2
#define default_servo_value 1500  //set the default servo value
#define PPM_PulseLen 300  //set the pulse length
#define PPM_FrLen (22500-PPM_PulseLen)  //set the PPM frame length in microseconds (1ms = 1000µs)
#define PPM_onState 1  //set polarity of the pulses: 1 is positive, 0 is negative

extern uint16_t ppm[PPM_number];

void ppm_start();
void ppm_stop();

#endif
