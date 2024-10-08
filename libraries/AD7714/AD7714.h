#ifndef AD7714_H
#define AD7714_H

#include <Arduino.h>
#include "../easySPI/easySPI.h"

/*
 * AD7714, AD-convertet attchment:
 * Supply: pin24-DGND, pin23-Vcc(+5)
 * Control: pin1-SCLK, pin22-Din, pin21-Dout, pin19-nCS, pin6-nReset(+5), pin20-nDRDY
 * OperationMode: pin4-Pol(+5), pin5-nSync(+5), pin11-nStandby(+5), pin13-Buffer(+5:on)
 * Clock: Pin2 & Pin3 - in/out Quartz rezonator 2.4576MHz
 * Reference: pin15-REFin+(to 2.5v), pin14-REFin-(to 0v)
 * Input: pin7,8,9,10,16,17 - ain1, ain2, ain3, ain4, ain5, ain6
 * Analog: pin18-AGND, pin12-AVcc(+5)
 */

class AD7714 : public easySPI {
public:
    //register selection  0x00
    //   7        6      5      4      3      2      1      0
    //0/DRDY(0) RS2(0) RS1(0) RS0(0) R/W(0) CH2(0) CH1(0) CH0(0)
    static const uint8_t REG_CMM    = 0x00; //communication register 8 bit
    static const uint8_t REG_MODE   = 0x10; //mode setup register 8 bit
    static const uint8_t REG_FILTH  = 0x20; //clock/filter register 8 bit
    static const uint8_t REG_FILTL  = 0x30; //filter register 8 bit
    static const uint8_t REG_TEST   = 0x40; //test register 8 bit
    static const uint8_t REG_DATA   = 0x50; //data register 16 or 24 bit, result
    static const uint8_t REG_OFFSET = 0x60; //offset register 24 bit
    static const uint8_t REG_GAIN   = 0x70; // gain register 24 bit

    static const uint8_t REG_RD     = 0x08; // CMM register command
    static const uint8_t REG_WR     = 0x00; // CMM register command
    static const uint8_t REG_nDRDY  = 0x80; // CMM register result

    //channel selection for AD7714
    //CH2 CH1 CH0                              +      -     cal. register pair 
    static const uint8_t CHN_16   = 0x0; //  AIN1+  AIN6-     #0
    static const uint8_t CHN_26   = 0x1; //  AIN2+  AIN6-     #1
    static const uint8_t CHN_36   = 0x2; //  AIN3+  AIN6-     #2
    static const uint8_t CHN_46   = 0x3; //  AIN4+  AIN6-     #2 
    static const uint8_t CHN_12   = 0x4; //  AIN1+  AIN2-     #0
    static const uint8_t CHN_34   = 0x5; //  AIN3+  AIN4-     #1
    static const uint8_t CHN_56   = 0x6; //  AIN5+  AIN6-     #2
    static const uint8_t CHN_TEST = 0x7; //  AIN6   AIN6 test #2
    // (AIN1) PIN#7    (AIN6) PIN#17
    // (AIN2) PIN#8    (AIN5) PIN#16
    // (AIN3) PIN#9     REF+  PIN#15
    // (AIN4) PIN#10    REF-  PIN#14

    //Mode Register   0x10
    //  7     6     5     4     3      2      1      0
    //MD2(0)MD1(0)MD0(0) G2(0) G1(0) G0(0)  B/O(0) FSYNC(0)
    //operating mode options
    //MD2 MD1 MD0
    static const uint8_t MODE_NORMAL         = 0x00; // normal operation mode
    static const uint8_t MODE_SELF_CAL       = 0x20; // self-calibration (6 period)
    static const uint8_t MODE_ZERO_SCALE_CAL = 0x40; // zero-scale system calibration (3)
    static const uint8_t MODE_FULL_SCALE_CAL = 0x60; // full-scale system calibration (3)
    static const uint8_t MODE_SYS_OFFSET_CAL = 0x80; // system-offset calibration     (6)
    static const uint8_t MODE_BG_CALIBRATION = 0xA0; // background calibration (data period x6)
    static const uint8_t MODE_SELF_ZS_CAL    = 0xC0; // zero scale self-calibration   (3)
    static const uint8_t MODE_SELF_FS_CAL    = 0xE0; // full scale self-calibration   (3)

    //gain setting
    static const uint8_t GAIN_1    = 0x00;
    static const uint8_t GAIN_2    = 0x04;
    static const uint8_t GAIN_4    = 0x08;
    static const uint8_t GAIN_8    = 0x0C;
    static const uint8_t GAIN_16   = 0x10;
    static const uint8_t GAIN_32   = 0x14;
    static const uint8_t GAIN_64   = 0x18;
    static const uint8_t GAIN_128  = 0x1C;

    static const uint8_t BURNOUT   = 0x02;// 0-no current, 1-on, pass current over input (1uA)
    static const uint8_t FSYNC     = 0x01;// 1-reset filters, 0-start working


    //Filter High Register   0x20
    //   7         6       5        4        3        2      1      0
    //UNIPOLAR(0) WL24(0) BST(0) CLKDIS(0) FS(11)  FS(10)  FS(9)  FS(8)
    //FS: filter scale  relative fosc/128
    static const uint8_t CLK_DIS   = 0x10; // master clock disable bit
    static const uint8_t CLK_EN    = 0x00;
    static const uint8_t BST       = 0x20; // analog current boost, @2.45MHz, gain>4
    static const uint8_t WL24      = 0x40; // data length 24 bits
    static const uint8_t UNIPOLAR  = 0x80; // unipolar mode
    static const uint8_t BIPOLAR   = 0x00;

    static const uint8_t FILTH_MASK= 0x0F;
    static const uint8_t FILTL_MASK= 0xFF;

    //Filter Select ratio values (assume 2.4576MHz resonator):
    static const uint16_t  UPDATE_RATE_5   = 0xF00; // 19200/5   = 3840.0
    static const uint16_t  UPDATE_RATE_10  = 0x780; // 19200/10  = 1920.0
    static const uint16_t  UPDATE_RATE_15  = 0x500; // 19200/15  = 1280.0
    static const uint16_t  UPDATE_RATE_20  = 0x3C0; // 19200/20  = 960.0
    static const uint16_t  UPDATE_RATE_25  = 0x300; // 19200/25  = 768.0
    static const uint16_t  UPDATE_RATE_30  = 0x280; // 19200/30  = 640.0
    static const uint16_t  UPDATE_RATE_39x = 0x1F0; // 19200/38.7097 = 496.0
    static const uint16_t  UPDATE_RATE_40  = 0x1E0; // 19200/40  = 480.0
    static const uint16_t  UPDATE_RATE_50  = 0x180; // 19200/50  = 384.0
    static const uint16_t  UPDATE_RATE_60  = 0x140; // 19200/60  = 320.0 * default
    static const uint16_t  UPDATE_RATE_100 = 0x0C0; // 19200/100 = 192.0
    static const uint16_t  UPDATE_RATE_200 = 0x060; // 19200/200 = 96.0
    static const uint16_t  UPDATE_RATE_250x= 0x04C; // 19200/250 = 76.8
    static const uint16_t  UPDATE_RATE_256 = 0x04B; // 19200/256 = 75.0
    static const uint16_t  UPDATE_RATE_500x= 0x026; // 19200/500 = 38.4
    static const uint16_t  UPDATE_RATE_1000x=0x013; // 19200/500 = 19.2 x-not exact

    static const uint8_t  CALIBRATE   = 1;
    static const uint8_t  NOCALIBRATE = 0;

    void cs() {
        digitalWrite(cs_pin, LOW);
    };

    void nocs() {
        digitalWrite(cs_pin, HIGH);
    };

    AD7714(float vref, uint8_t cs = 10);
    uint16_t read16(uint8_t reg);
    uint32_t read24(uint8_t reg);
    void load24(uint8_t reg, uint32_t data);
    uint8_t command(uint8_t reg, uint8_t cmd);
    void reset();
    void wait(uint8_t channel);
    void fsync(uint8_t channel);
    float read(uint8_t channel);
    float conv(uint32_t code);
    bool ready(uint8_t channel);
    void self_calibrate(uint8_t channel);
    void init(uint8_t channel, uint8_t polarity=BIPOLAR, uint8_t gain=GAIN_1,
	uint16_t updRate=UPDATE_RATE_10, uint8_t calibrate=1); //assume 2.4576MHz
    uint16_t t9conv(uint32_t osc);
//private:
    float VRef;                        // attached to REF in+(pin9), REF in-(pin10)
    uint8_t adc_mode, adc_filth, adc_filtl;
    uint8_t cs_pin;
};
#endif
