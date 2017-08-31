#ifndef _eSPI_H_INCLUDED
#define _eSPI_H_INCLUDED

#include <stdio.h>
#include <Arduino.h>
#include <avr/pgmspace.h>

#define SPI_CLOCK_DIV4   0x00
#define SPI_CLOCK_DIV16  0x01
#define SPI_CLOCK_DIV64  0x02
#define SPI_CLOCK_DIV128 0x03

#define SPI_MODE0        0x00
#define SPI_MODE1        0x04
#define SPI_MODE2        0x08
#define SPI_MODE3        0x0C

class easySPI {
public:
//easySPI::
easySPI(byte modeclk)
{
  pinMode(pnSCK, OUTPUT);
  pinMode(pnMOSI, OUTPUT);
  pinMode(pnSS, OUTPUT);
  pinMode(pnMISO, INPUT);

  _spi_mode = modeclk | _BV(MSTR) | _BV(SPE);
  spi_begin();
};
//  easySPI(byte modeclk);

  inline static byte spi_send(byte _data)
      { for(SPDR = _data;!(SPSR & _BV(SPIF));); return SPDR; }

  void spi_begin()
      { SPCR = _spi_mode; }

  void spi_end()
      { SPCR &= ~_BV(SPE);  }

private:
    static const int pnMOSI = 11;
    static const int pnMISO = 12;
    static const int pnSCK = 13; 
    static const int pnSS  = 10; 
    uint8_t _spi_mode;
};


#endif
