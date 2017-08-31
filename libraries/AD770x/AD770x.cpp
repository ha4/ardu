/*
 * AD7705/AD7706 Library
 */

#include "AD770X.h"

unsigned int AD770X::read16(uint8_t reg)
{
    unsigned int r;
    cs();
    spi_send(reg);
    r = spi_send(0xff);
    r = (r << 8) | spi_send(0xff);
    nocs();

    return r;
}

void AD770X::command(uint8_t reg, uint8_t cmd)
{
    cs();
    spi_send(reg);
    spi_send(cmd);
    nocs();
}

// more high level

void AD770X::reset()
{ 
    cs();
    for (int i = 0; i < 8; i++)
        spi_send(0xff);
    nocs();
}

bool AD770X::ready(uint8_t channel)
{
    cs();
    spi_send(REG_CMM | channel | REG_RD);
    uint8_t b1 = spi_send(0xff);
    nocs();

    return (b1 & 0x80) == 0x0;
}

void AD770X::wait(uint8_t channel) {
    uint8_t b1;
    cs();
    do {
        spi_send(REG_CMM | channel | REG_RD);
        b1 = spi_send(0xff);
    } while (b1 & 0x80);
    nocs();
}

float AD770X::read(uint8_t channel) {
    unsigned int h;

    wait(channel);
    h = read16(REG_DATA|channel|REG_RD);
    return  conv(h);
}

float AD770X::conv(uint16_t code) {
    uint8_t d = 1 << ((adc_mode & GAIN_128)>>3);
    if (adc_mode & UNIPOLAR)
		return  (code / 65536.0) * VRef / (float)d;
    else
		return  (code / 32768.0 - 1.0) * VRef / (float)d;
}

AD770X::AD770X(float vref, uint8_t cs):easySPI(SPI_MODE3|SPI_CLOCK_DIV16)
{
    VRef = vref;
    cs_pin = cs;
    pinMode(cs, OUTPUT);
    nocs();
}

void AD770X::init(uint8_t channel, uint8_t polarity, uint8_t gain,
	uint8_t clkDivider, uint8_t updRate)
{

    //ZERO(0) ZERO(0) ZERO(0) CLKDIS(0) CLKDIV(0) CLK(1) FS1(0) FS0(1)
    command(REG_CLOCK|channel|REG_WR,  clkDivider|updRate);

    // FSYNC
    //MD1(0) MD0(0) G2(0) G1(0) G0(0) B/U(0) BUF(0) FSYNC(1)
    adc_mode=MODE_NORMAL|gain|polarity;
    command(REG_SETUP|channel|REG_WR, adc_mode);
    wait(channel);

    // SELF_CALIBRATE
    command(REG_SETUP|channel|REG_WR, MODE_SELF_CAL|gain|polarity);
    wait(channel);
}


