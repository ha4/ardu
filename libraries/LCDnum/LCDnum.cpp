#include "LCDnum.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "Arduino.h"

LCDnum::LCDnum(uint8_t cs, uint8_t base):easySPI(SPI_MODE0|SPI_CLOCK_DIV16)
{

  _cs_pin = cs;
  _base_pin = base;
  _flag = LCDN_ON | LCDN_LEFT | LCDN_SCRL; 

  pinMode(_cs_pin, OUTPUT);
  pinMode(_base_pin, OUTPUT);
  digitalWrite(_cs_pin, 1);
  digitalWrite(_base_pin, 0);

  clear();
  spi_begin();
}

void LCDnum::refresh(uint8_t flash_clk)
{
  if (!(_flag & LCDN_ON)) return;

  uint8_t x;
  int8_t  i;
  x = digitalRead(_base_pin)?0:0xff;
  digitalWrite(_cs_pin, 0);
  if(_flag & LCDN_LEFT)
   for(i=LCDN_MIN;i<=LCDN_MAX;i++)
	spi_send(_data[i] ^ x);
  else
   for(i=LCDN_MAX;i>=LCDN_MIN;i--)
	spi_send(_data[i] ^ x);
  digitalWrite(_cs_pin, 1);
  digitalWrite(_base_pin, x & 1);
}

/********** high level commands, for the user! */
void LCDnum::clear()
{
   int8_t i;

   for(i=LCDN_MIN;i<=LCDN_MAX;i++)
	_data[i] = 0;
   home();
}

void LCDnum::home()
{
  _flag &= ~LCDN_PTSET;
  _curridx = LCDN_MIN;
}

void LCDnum::noDisplay() {
  int8_t i;

  _flag &= ~LCDN_ON;
  digitalWrite(_cs_pin, 0);
   for(i=LCDN_MIN;i<=LCDN_MAX;i++)
	spi_send(0);
  digitalWrite(_cs_pin, 1);
  digitalWrite(_base_pin, 0);
}
void LCDnum::display() {
  _flag |= LCDN_ON;
}

void LCDnum::scrollDisplay(void) {
 _data[0]=_data[1];
 _data[1]=_data[2];
 _data[2]=_data[3];
 _data[3]=0;
}

void LCDnum::unScroll(void) {
 _data[3]=_data[2];
 _data[2]=_data[3];
 _data[1]=_data[1];
 _data[0]=0;
}

void LCDnum::leftToRight(void) {
  _flag |= LCDN_LEFT;
}

void LCDnum::rightToLeft(void) {
  _flag &= ~LCDN_LEFT;
}

void LCDnum::autoscroll(void) {
  _flag |= LCDN_SCRL;
}

void LCDnum::noAutoscroll(void) {
  _flag &= ~LCDN_SCRL;
}

// void LCDnum::createChar(uint8_t location, uint8_t charbits) {
// }
static uint8_t _numidx[] = {
	LCDN_0, LCDN_1, LCDN_2, LCDN_3, LCDN_4,
	LCDN_5, LCDN_6, LCDN_7, LCDN_8, LCDN_9 
};

static uint8_t _alphidx[] = {
	LCDN_A, LCDN_B, LCDN_C, LCDN_D, LCDN_E, LCDN_F,
};

inline size_t LCDnum::write(uint8_t value) {
  switch(value) {
    case '\n': _curridx=0xff; return 1;
    case '\r': _curridx=0xff; return 1;
    case ' ':  value = LCDN_SPACE; goto putchrx;
    case '-':  value = LCDN_MINUS; goto putchrx;
    case '.':  case ',':
	if (_flag & LCDN_PTSET) { value=LCDN_COMMA; goto putchr; }
  	_flag |= LCDN_PTSET;
       	value = prev();
	_data[value] |= LCDN_COMMA;
	return 1;
    }

    if (value >='0' && value <='9')
	value = _numidx[value-'0'];
    else if (value >='A' && value <='F')
	value = _alphidx[value-'A'];
    else if (value >='a' && value <='f')
	value = _alphidx[value-'a'];
    else
	value = LCDN_UNDER;
  putchrx:
    _flag &= ~LCDN_PTSET;

  putchr:
    if (_curridx==0xff) clear();

    if (_curridx > LCDN_MAX && (_flag & LCDN_SCRL)) {
	scrollDisplay();
	_curridx = prev();
    }

    if (_curridx <= LCDN_MAX) {
	_data[_curridx]=value;
       	_curridx = next();
    }
    return 1; // assume sucess
}


uint8_t  LCDnum::prev()
{
  return _curridx - 1;
}

uint8_t  LCDnum::next()
{
  return _curridx + 1;
}

