#ifndef __INPUT_TXER_H_
#define __INPUT_TXER_H_

#define POT_THROTTLE 1
#define POT_RUDDER   3 /* yaw */
#define POT_ELEVATOR 2 /* pitch */
#define POT_AILERON  0 /* roll */

#define ADC_SIZE 8
#define ADC_UREF 1106 /*mV*/
#define ADC_KBAT 0.01854
#define ADC_SBAT 0.5
#define ADC_UBAT10(c) (5+(((c)*12150L)>>16))

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
#define KEY_LU 0x1000
#define KEY_LD 0x4000
#define KEY_RU 0x8000
#define KEY_RD 0x2000
#define KEY_R  0x0100
#define KEY_L  0x0400
#define KEY_SYS (KEY_1|KEY_2|KEY_3|KEY_4)

void kscan_init();
void kscan_tick();
uint16_t kscan_keys();

uint16_t kscan_mx();


#endif
