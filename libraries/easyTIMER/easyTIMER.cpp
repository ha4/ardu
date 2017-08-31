#include "pins_arduino.h"
#include "easyTIMER.h"

easyTIMER::easyTIMER(uint8_t source, uint8_t mode, uint8_t irqmode)
{
   clk_stop();
   _src = source;
   _mode = mode;
   _timsk = irqmode;
}

void easyTIMER::clk_seta(uint16_t reg)
{
  noInterrupts();
  OCR1AH = reg>>8;
  OCR1AL = reg&0xFF;
  interrupts();
}

void easyTIMER::clk_setb(uint16_t reg)
{
  noInterrupts();
  OCR1BH = reg>>8;
  OCR1BL = reg&0xFF;
  interrupts();
}

void easyTIMER::clk_seti(uint16_t reg)
{
  noInterrupts();
  ICR1H = reg>>8;
  ICR1L = reg&0xFF;
  interrupts();
}

void easyTIMER::clk_start()
{
  noInterrupts();
  TCCR1A = _mode & 0x03;
  TCCR1B = ((_mode & 0x0C) << 1 ) | _src;
  TIMSK1 = _timsk; 
  interrupts();
}

void easyTIMER::clk_stop()
{
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  interrupts();
}
