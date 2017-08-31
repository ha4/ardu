#ifndef AD770X_H
#define AD770X_H

#include <Arduino.h>
#include "../easySPI/easySPI.h"

/*
 * AD770X, AD-convertet attchment:
 * Supply: pin16-GND, pin15-Vcc(+5)
 * Control: pin1-SCLK, pin14-Din, pin13-Dout, pin4-CS, pin5-Reset(to Vcc), pin12-DRDY(not use)
 * Clock: Pin2 & Pin3 - Quartz rezonator 2.0MHz
 * Reference: pin9-REFin+(to 2.5v), pin10-REFin-(to 0v)
 * Input (AD7705): pin6,7,8,11 - ain2(+), ain1(+), ain1(-), ain2(-)
 * Input (AD7706): pin6,7,8,11 - ain1, ain2, common, ain3
 */

class AD770X : public easySPI {
public:
    //register selection  0x00
    //   7        6      5      4      3      2      1      0
    //0/DRDY(0) RS2(0) RS1(0) RS0(0) R/W(0) STBY(0) CH1(0) CH0(0)
    static const uint8_t REG_CMM    = 0x00; //communication register 8 bit
    static const uint8_t REG_SETUP  = 0x10; //setup register 8 bit
    static const uint8_t REG_CLOCK  = 0x20; //clock register 8 bit
    static const uint8_t REG_DATA   = 0x30; //data register 16 bit, contains conversion result
    static const uint8_t REG_TEST   = 0x40; //test register 8 bit, POR 0x0
    static const uint8_t REG_NOP    = 0x50; //no operation
    static const uint8_t REG_OFFSET = 0x60; //offset register 24 bit
    static const uint8_t REG_GAIN   = 0x70; // gain register 24 bit

    static const uint8_t REG_RD     = 0x08; // gain register 24 bit
    static const uint8_t REG_WR     = 0x00; // gain register 24 bit

    //channel selection for AD7706 
    //CH1 CH0
    static const uint8_t CHN_AIN1    = 0x0; //AIN1(pin6); calibration register pair 0
    static const uint8_t CHN_AIN2    = 0x1; //AIN2(pin7); calibration register pair 1
    static const uint8_t CHN_COMM    = 0x2; //common(pin8); calibration register pair 0
    static const uint8_t CHN_AIN3    = 0x3; //AIN3(pin11); calibration register pair 2
    //channel selection for AD7705
    static const uint8_t CHN_AIN1p1m = 0x0; //AIN1+(pin7) AIN1-(pin8); calibration register pair 0
    static const uint8_t CHN_AIN2p2m = 0x1; //AIN2+(pin6) AIN2-(oin11); calibration register pair 1
    static const uint8_t CHN_AIN1m1m = 0x2; //AIN1-(pin8) AIN1-(pin8); calibration register pair 0
    static const uint8_t CHN_AIN1m2m = 0x3; //AIN1-(pin8) AIN2-(pin11); calibration register pair 2


    //Clock Register   0x20
    //   7      6       5        4        3        2      1      0
    //ZERO(0) ZERO(0) ZERO(0) CLKDIS(0) CLKDIV(0) CLK(1) FS1(0) FS0(1)
    //CLKDIS: master clock disable bit
    //CLKDIV: clock divider by 2
    //CLK:    clock select 1:2.4576Mhz, 0:2.0000Mhz
    //FS1 FS0: filter select
    //output update rate
    static const uint8_t UPDATE_RATE_20   = 0x00; // 20 Hz   clk:2.0000MHz
    static const uint8_t UPDATE_RATE_25   = 0x01; // 25 Hz
    static const uint8_t UPDATE_RATE_100  = 0x02; // 100 Hz
    static const uint8_t UPDATE_RATE_200  = 0x03; // 200 Hz
    static const uint8_t UPDATE_RATE_50   = 0x04; // 50 Hz   clk:2.4576MHz
    static const uint8_t UPDATE_RATE_60   = 0x05; // 60 Hz
    static const uint8_t UPDATE_RATE_250  = 0x06; // 250 Hz
    static const uint8_t UPDATE_RATE_500  = 0x07; // 500 Hz

    static const uint8_t CLK_DIV_1        = 0x00;
    static const uint8_t CLK_DIV_2        = 0x08;

    static const uint8_t CLK_DIS          = 0x10;
    static const uint8_t CLK_EN           = 0x00;

    //Setup Register   0x10
    //  7     6     5     4     3      2      1      0
    //MD1(0) MD0(0) G2(0) G1(0) G0(0) B/U(0) BUF(0) FSYNC(1)
    //operating mode options
    //MD1 MD0
    static const uint8_t MODE_NORMAL         = 0x00; //normal mode
    static const uint8_t MODE_SELF_CAL       = 0x40; //self-calibration
    static const uint8_t MODE_ZERO_SCALE_CAL = 0x80; //zero-scale system calibration, POR 0x1F4000, set FSYNC high before calibration, FSYNC low after calibration
    static const uint8_t MODE_FULL_SCALE_CAL = 0xC0; //full-scale system calibration, POR 0x5761AB, set FSYNC high before calibration, FSYNC low after calibration

    //gain setting
    static const uint8_t GAIN_1    = 0x00;
    static const uint8_t GAIN_2    = 0x08;
    static const uint8_t GAIN_4    = 0x10;
    static const uint8_t GAIN_8    = 0x18;
    static const uint8_t GAIN_16   = 0x20;
    static const uint8_t GAIN_32   = 0x28;
    static const uint8_t GAIN_64   = 0x30;
    static const uint8_t GAIN_128  = 0x38;

    static const uint8_t UNIPOLAR  = 0x04;
    static const uint8_t BIPOLAR   = 0x00;

    static const uint8_t BUFFERED  = 0x02;
    static const uint8_t UNBUFFERED= 0x00;

    static const uint8_t FSYNC     = 0x01;


    void cs() {
        digitalWrite(cs_pin, LOW);
    };

    void nocs() {
        digitalWrite(cs_pin, HIGH);
    };

    AD770X(float vref, uint8_t cs = 10);
    uint16_t read16(uint8_t reg);
    void command(uint8_t reg, uint8_t cmd);
    void reset();
    void wait(uint8_t channel);
    float read(uint8_t channel);
    float conv(uint16_t code);
    bool ready(uint8_t channel);
    void init(uint8_t channel, uint8_t polarity=BIPOLAR, uint8_t gain=GAIN_1,
	uint8_t clkDivider=CLK_DIV_2, uint8_t updRate=UPDATE_RATE_25);
private:
    float VRef;                        // attached to REF in+(pin9), REF in-(pin10)
    uint8_t adc_mode;
    uint8_t cs_pin;
};
#endif
