#include <Arduino.h>
#include "inp_x.h"

static uint16_t k_matrix, k_matrix0;
static uint16_t k_debouncing;
static uint16_t k_cnt;

#ifndef K_DEBOUNCE
#   define K_DEBOUNCE  3*8
#endif
static uint8_t debouncing;

void kscan_init()
{
  k_cnt = 0;
  k_matrix0 = 0;
  k_matrix = 0;
  k_debouncing = 0;
  debouncing = K_DEBOUNCE;
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
  if (digitalRead(COLPINA)) k_matrix0 &= ~k_cnt; else k_matrix0 |= k_cnt; k_cnt <<= 1;
  if (digitalRead(COLPINB)) k_matrix0 &= ~k_cnt; else k_matrix0 |= k_cnt; k_cnt <<= 1;
}

uint16_t kscan_keys()
{
  if (k_matrix0 != k_debouncing) {
    k_debouncing=k_matrix0;
    debouncing = K_DEBOUNCE;
  }
  if (debouncing) debouncing--;
  else k_matrix = k_debouncing;

  return k_matrix;
}

uint16_t kscan_mx()
{
  do kscan_tick(); while (k_cnt != 0);
  return k_matrix0;
}
