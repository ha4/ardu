/*
 * AD7714 Library
 */

#include "AD7714.h"

uint16_t AD7714::read16(uint8_t reg)
{
    uint16_t r;
    cs();
    spi_send(reg);
    r = spi_send(0xff);
    r = (r << 8) | spi_send(0xff);
    nocs();

    return r;
}

uint32_t AD7714::read24(uint8_t reg)
{
    uint32_t r;
    cs();
    spi_send(reg);
    r = spi_send(0xff);
    r = (r << 8) | spi_send(0xff);
    r = (r << 8) | spi_send(0xff);
    nocs();

    return r;
}

void AD7714::load24(uint8_t reg, uint32_t data)
{
    cs();
    spi_send(reg);
    spi_send(0xff & (data >>16));
    spi_send(0xff & (data >>8));
    spi_send(0xff &  data);
    nocs();
}

uint8_t AD7714::command(uint8_t reg, uint8_t cmd)
{
    uint8_t r;

    cs();
    spi_send(reg);
    r = spi_send(cmd);
    nocs();
    if (reg & REG_RD) cmd = r;
    switch(reg & 0x70) {
    case REG_MODE: adc_mode = cmd; break;
    case REG_FILTH: adc_filth = cmd; break;
    case REG_FILTL: adc_filtl = cmd; break;
    }
    return r;
}

// more high level

void AD7714::reset()
{ 
    cs();
    for (int i = 0; i < 8; i++)
        spi_send(0xff);
    nocs();
}

bool AD7714::ready(uint8_t channel)
{
    cs();
    spi_send(REG_CMM | channel | REG_RD);
    uint8_t b1 = spi_send(0xff);
    nocs();

    return (b1 & 0x80) == 0x0;
}

void AD7714::wait(uint8_t channel) 
{
    uint8_t b1;
    cs();
    do {
        spi_send(REG_CMM | channel | REG_RD);
        b1 = spi_send(0xff);
    } while (b1 & REG_nDRDY);
    nocs();
}

void AD7714::fsync(uint8_t channel)
{
    cs();
    command(REG_MODE|channel|REG_WR,  adc_mode | FSYNC);
    command(REG_MODE|channel|REG_WR,  adc_mode & (~FSYNC));
    nocs();
}

float AD7714::read(uint8_t channel)
{
    uint32_t h;

    wait(channel);
    if (adc_filth & WL24)
	    h = read24(REG_DATA|channel|REG_RD);
    else
	    h = read16(REG_DATA|channel|REG_RD);
    return  conv(h);
}

float AD7714::conv(uint32_t code)
{
    uint8_t d = 1 << ((adc_mode & GAIN_128)>>2);
    float sc, ba;
    if (adc_filth & WL24)
	sc=16777216.0;
    else 
	sc=65536.0;
    if (adc_filth & UNIPOLAR)
	ba=0;
    else {
	sc/=2.0; ba=1.0;
    }
    return (code / sc - ba) * VRef / (float)d;
}

void AD7714::self_calibrate(uint8_t channel)
{
    command(REG_MODE|channel|REG_WR,  adc_mode | MODE_SELF_CAL);
    wait(channel);
}

void AD7714::init(uint8_t channel, uint8_t polarity, uint8_t gain,
	uint16_t filtRatio, uint8_t calibrate)
{
    adc_filth = polarity | WL24 | BST | ((filtRatio>>8)& FILTH_MASK); // clkdis=0
    adc_filtl = filtRatio & FILTL_MASK;
    adc_mode=MODE_NORMAL|gain; // fsync=0, burnout=0
    command(REG_FILTH|channel|REG_WR,  adc_filth);
    command(REG_FILTL|channel|REG_WR,  adc_filtl);
    if(calibrate) self_calibrate(channel);
    command(REG_MODE|channel|REG_WR, MODE_NORMAL|gain);
}

uint16_t AD7714::t9conv(uint32_t osc)
{
  uint32_t fosc;
  uint32_t fs;
  fosc = osc/128;
  fs = ((adc_filth&0x0F) << 8) | adc_filtl;
  return 9000*fs/fosc;
}


AD7714::AD7714(float vref, uint8_t cs):easySPI(SPI_MODE3|SPI_CLOCK_DIV16)
{
    VRef = vref;
    cs_pin = cs;
    pinMode(cs, OUTPUT);
    nocs();
}
