#ifndef __INPUT_TXER_H_
#define __INPUT_TXER_H_

#define POT_THROTTLE 1
#define POT_RUDDER   3 /* yaw */
#define POT_ELEVATOR 2 /* pitch */
#define POT_AILERON  0 /* roll */

#define ADC_SIZE 8
#define ADC_UREF 1106 /*mV*/
uint16_t adc_read(uint8_t chan);
void adc_setup();

#define DATAPIN 8
#define CLKPIN  6
#define COLPINA 5
#define COLPINB 7

#define KEY_1 8
#define KEY_2 2
#define KEY_3 4
#define KEY_4 1

void kscan_init();
void kscan_tick();
uint16_t kscan_keys();

uint16_t kscan_mx();


#endif
