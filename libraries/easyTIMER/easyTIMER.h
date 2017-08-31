#ifndef _eTIMER_H_INCLUDED
#define _eTIMER_H_INCLUDED

#include <stdio.h>
#include <Arduino.h>
#include <avr/pgmspace.h>

#define CLK_OFF         0x00
#define CLK_OSC         0x01
#define CLK_OSC_DIV8    0x02
#define CLK_OSC_DIV64   0x03
#define CLK_OSC_DIV256  0x04
#define CLK_OSC_DIV1024 0x05
#define CLK_EXT_FALLING 0x06
#define CLK_EXT_RISING  0x07

// TIMER1 modes:
// NORMAL - time counter with overflow
// CTC - compare time counter
// FPWM - fast PWM
// PWM - phase correct PWM
// PWM_PFC - phase and frequency correct PWM
// 
#define CLK_M0_NORMAL  0x00    /* top: 0xFFFF */
#define CLK_M1_PWM8    0x01    /* top: 0x00FF */
#define CLK_M2_PWM9    0x02    /* top: 0x01FF */
#define CLK_M3_PWM10   0x03    /* top: 0x03FF */
#define CLK_M4_CTC     0x04  /* top: A */
#define CLK_M5_FPWM8   0x05    /* top: 0x00FF */
#define CLK_M6_FPWM9   0x06    /* top: 0x01FF */
#define CLK_M7_FPWM10  0x07    /* top: 0x03FF */
#define CLK_M8_PWM_PFC 0x08  /* top: I */
#define CLK_M9_PWM_PFC 0x09  /* top: A */
#define CLK_M10_PWM16  0x0A  /* top: I */
#define CLK_M11_PWM16  0x0B  /* top: A */
#define CLK_M12_CTC    0x0C  /* top: I */
#define CLK_M14_FPWM   0x0E  /* top: I */
#define CLK_M15_FPWM   0x0F  /* top: A */

#define CLK_IRQ_OVERFLOW 0x01
#define CLK_IRQ_MATCHA   0x02
#define CLK_IRQ_MATCHB   0x04
#define CLK_IRQ_INPUT    0x20

class easyTIMER {
public:
  easyTIMER(uint8_t source, uint8_t mode, uint8_t irqmode);
  void clk_seta(uint16_t reg);
  void clk_setb(uint16_t reg);
  void clk_seti(uint16_t reg);
  void clk_start();
  void clk_stop();
  
private:
    uint8_t _src, _mode, _timsk;
};


#endif
