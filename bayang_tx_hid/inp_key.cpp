#include <Arduino.h>
#include "inp_x.h"

static uint16_t k_matrix;
static uint8_t k_cnt;

void kscan_init()
{
  k_cnt = 0;
  k_matrix = 0;
  pinMode(DATAPIN, OUTPUT);
  pinMode(CLKPIN, OUTPUT);
  pinMode(COLPINA, INPUT_PULLUP);
  pinMode(COLPINB, INPUT_PULLUP);
}

void kscan_tick()
{
  if (k_cnt == 0) k_cnt = 1;
  digitalWrite(CLKPIN, 0);
  digitalWrite(DATAPIN, (k_cnt == 1) ? 0 : 1);
  digitalWrite(CLKPIN, 1);
  delayMicroseconds(2);
  if (digitalRead(COLPINA)) k_matrix &= ~k_cnt; else k_matrix |= k_cnt; k_cnt <<= 1;
  if (digitalRead(COLPINB)) k_matrix &= ~k_cnt; else k_matrix |= k_cnt; k_cnt <<= 1;
}

uint16_t kscan_mx()
{
  do kscan_tick(); while (k_cnt != 0);
  return k_matrix;
}
