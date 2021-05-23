#ifndef __INPUT_TXER_H_
#define __INPUT_TXER_H_

#define POT_THROTTLE 2
#define POT_RUDDER   0
#define POT_ELEVATOR 1
#define POT_AILERON  3

uint16_t adc_read(uint8_t chan);
void adc_setup();

// 74hc164 pin 1&2 - DATA, 8 - CLOCK, pin 9 - CLEAR (+Vdd)
#define DATAPIN 8
#define CLKPIN  6
#define COLPINA 5
#define COLPINB 7

void kscan_init();
void kscan_tick();
uint16_t kscan_mx();


#endif
