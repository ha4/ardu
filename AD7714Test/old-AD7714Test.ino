/*
 * AD7714 Library
 * sample code
 * xommands:
 z[10]  zero scale calibration enable
 f[01]  full scale calibration enable
 a[01]  ADC enable measurement
 c[0-7] select ADC channel
 r<decimal16> ratio set: [fosc/128]:[sample-ratio]
 s[312]      system of calibration: 1-system cal, 2-self cal, 3-both
 w<decimal32> write offset register
 W<decimal32> write gain regster
 d{80}  delay before wait drdy cycle
 y[10]  fsync @ ADC start 
 e[10]  enable relay switch zs/fs
 i[340] item relaty 3=zero, 4=fullscale, 0=off
 */

// DC-cut: Y(i) = X(i) + 0.875Y(i-1) - X(i-1)
//         Y(i) = X(i) + (1-0.128)Y(i-1) - X(i-1)
//         Y(i) = X(i) + Y(i-1) - Y(i-1)/8 - X(i-1)
//         Y(i) += X(i) - Y(i-1)/8 - X(i-1)

//DEFAULT:
// ZS:1F4000=2048000
// FS:5761AB=5726635
// TEST RESULT1:
// ch4 flt=1250, Vref=2.5v
// offset: ZS = ZSself - 450
// gain:   FS = FSself * 1.0002441149127882408773834521944
// gain:   FS = FSself + 2.441149128e-4*FSself // 16/65536
// TEST RESULT2:
// ch4 flt=1250, Vref=2.5v
// ZScorr=425 FScorr=256ppm
// TEST RESULT3:
// ch6 flt=1250, Vref=2.5v
// ZScorr=338 FScorr=271ppm
// TEST RESULT4:
// ch5 flt=1250, Vref=2.5v
// ZScorr=415 FScorr=233ppm
// TEST RESULT5:
// ch4 flt=1250, Vref=2.5v
// ZScorr=347 FScorr=620ppm


#include <AD7714.h>

enum {
    swzero=3, swfull=4, clko=9
};

AD7714 adc(2.5);
int sys, zsc, fsc, chc, flt, measurev;
int dly, syn, swen;
uint32_t acc_data[5];
uint32_t acc_cntr[5];


void setup()
{
  Serial.begin(57600);

  // PB1, OC1A, D9 pin 2MHz clock, fosc/128=15625Hz
  //       mode 0100 (CTC),top=OCR1A, COM1A mode=1 (toggle), src=001
  TCCR1A = (0b01 << COM1A0)| (0b00 << WGM10); // div 2
  TCCR1B = (0b01 << WGM12) | (0b001 << CS10); // clk/1 (16MHz)
  OCR1A = 4 - 1; // div 4
  pinMode(clko, OUTPUT);

  //relay
  pinMode(swzero, OUTPUT);
  pinMode(swfull, OUTPUT);
  digitalWrite(swzero,0);  
  digitalWrite(swfull,0);  

  acc_reset();
  syn = 1;
  swen = 1;
  sys = 3;
  zsc = 1;
  fsc = 0;
  measurev=0;
  chc = AD7714::CHN_12;
  flt = 1250; // 2MHz/128 = 15635Hz. SR=15625/1250=12.5Hz 
  dly = 80; // 80ms

  Serial.println(":start");

  adc.reset();
  adc.init(chc, AD7714::BIPOLAR,AD7714::GAIN_1, flt, AD7714::NOCALIBRATE); 
  Serial.println(":init-done");
}

void acc_reset()
{
   int n;
   for (n=0; n < 5; n++){acc_data[n]=0; acc_cntr[n]=0; }
}

void acc_show()
{
    uint32_t zsys,zself,fsys,fself,v;

    v    = acc_data[0]/acc_cntr[0];
    zsys = acc_data[1]/acc_cntr[1];
    zself= acc_data[2]/acc_cntr[2];
    fsys = acc_data[3]/acc_cntr[3];
    fself= acc_data[4]/acc_cntr[4];
    Serial.print("{");
    Serial.print("nsum=");
    Serial.print(acc_cntr[0]+acc_cntr[1]+acc_cntr[2]+acc_cntr[3]+acc_cntr[4]);
    Serial.print(" v ");
    Serial.print(((double)v)/1e7);
    Serial.print(" offs ");
    Serial.print(zsys);
    Serial.print(" ");
    Serial.print(zself);
    Serial.print(" gain ");
    Serial.print(fsys);
    Serial.print(" ");
    Serial.print(fself);
    Serial.print(" delta ");
    Serial.print(zsys-zself);
    Serial.print(" ");
    Serial.print(fsys-fself);
    Serial.print(" (k-1)*1e6 ");
    Serial.print(((fsys-fself)*1000000)/fself);
    Serial.print("}");
}

void acc_add(uint8_t m, uint32_t v)
{
  int n;

  switch(m) {
  case AD7714::MODE_ZERO_SCALE_CAL: n=1; break;
  case AD7714::MODE_SELF_ZS_CAL: n=2; break;
  case AD7714::MODE_FULL_SCALE_CAL: n=3; break;
  case AD7714::MODE_SELF_FS_CAL: n=4; break;
  default: n = 0;
  }
  acc_cntr[n]++;
  acc_data[n]+=v;
}

void adc_show(uint8_t ch)
{
    int32_t v;
    uint8_t m;

    adc.cs();

    adc.spi_send(AD7714::REG_CMM|AD7714::REG_RD|ch);
    v=adc.spi_send(0xFF); Serial.print("{cmm 0x"); Serial.print(v,HEX);
    adc.spi_send(AD7714::REG_MODE|AD7714::REG_RD|ch);
    v=adc.spi_send(0xFF); Serial.print(" mode 0x"); Serial.print(v,HEX);

    adc.spi_send(AD7714::REG_FILTH|AD7714::REG_RD|ch);
    m=adc.spi_send(0xFF); Serial.print(" filt-h 0x"); Serial.print(m,HEX);
    
    adc.spi_send(AD7714::REG_FILTL|AD7714::REG_RD|ch);
    v=adc.spi_send(0xFF); Serial.print(" filt-l 0x"); Serial.print(v,HEX);

    v = adc.read24(AD7714::REG_OFFSET|AD7714::REG_RD|ch);
    Serial.print(" offset "); Serial.print(v);
    v = adc.read24(AD7714::REG_GAIN|AD7714::REG_RD|ch);
    Serial.print(" gain "); Serial.print(v);
    v = adc.read24(AD7714::REG_DATA|AD7714::REG_RD|ch);
    Serial.print(" data ");
    if (m&AD7714::UNIPOLAR) Serial.print(v);
       else         Serial.print(8388608-v);
    Serial.print("}");
}

void aloop(uint8_t ch)
{
  double v;

  if (syn) adc.fsync(ch);
  if(dly > 0) delay((syn)?3*dly:dly);
  v = adc.read(ch);
  if (v>=0)   Serial.print('+');
  Serial.print(v,7);
  acc_add(0,1e7*v);
}

// calibrate
void cloop(uint8_t chan, uint8_t swpin, uint8_t mode, uint8_t reg)
{
  uint32_t cc;

  if (swen) {  swch(swpin); delay(100); }

  adc.command(AD7714::REG_MODE|chan|AD7714::REG_WR, mode|AD7714::GAIN_1);
  if(dly > 0) delay(dly*3);
  adc.wait(chan);
  cc = adc.read24(reg|chan|AD7714::REG_RD);
  Serial.print(cc);
  acc_add(mode,cc);
}

void adc_load24(uint8_t reg, uint32_t data)
{
    adc.cs();
    adc.spi_send(reg);
    adc.spi_send(0xff & (data >>16));
    adc.spi_send(0xff & (data >>8));
    adc.spi_send(0xff &  data);
    adc.nocs();
}

void iloop(void)
{
  uint32_t h;
  if (!Serial.available()) return;
  switch(Serial.read()){
    case 'z':    zsc = Serial.parseInt();    Serial.print("zeroscale-cal");    Serial.print(zsc);    break;
    case 'f':    fsc=Serial.parseInt();    Serial.print("fullscale-cal");    Serial.print(fsc);    break;
    case 'a':    measurev=Serial.parseInt();    Serial.print("adc-measure");    Serial.print(measurev);    break;
    case 'c':    chc=Serial.parseInt();    Serial.print("channel");    Serial.print(chc);    break;
    case 'r':    h=Serial.parseInt();    Serial.print("ratio");    Serial.print(h);
                adc.init(chc, AD7714::BIPOLAR,AD7714::GAIN_1, h, AD7714::NOCALIBRATE); 
                break;
    case 's':    sys=Serial.parseInt();    Serial.print("calibration-system");    Serial.print(sys);    break;
    case 'w':    h=Serial.parseInt();    Serial.print("load-offset");    Serial.print(h);
                adc_load24(chc|AD7714::REG_WR|AD7714::REG_OFFSET, h); break;
    case 'W':    h=Serial.parseInt();    Serial.print("load-gain");    Serial.print(h);
                adc_load24(chc|AD7714::REG_WR|AD7714::REG_GAIN, h); break;
    case 'd':    dly=Serial.parseInt();    Serial.print("delay-begfore-wait[ms]");    Serial.print(dly); break;
    case 'y':    syn=Serial.parseInt();    Serial.print("fsync");    Serial.print(syn); break;
    case 'e':    swen=Serial.parseInt();    Serial.print("switch-enable");    Serial.print(swen); break;
    case 'i':    h=Serial.parseInt();    Serial.print("switch");    Serial.print(h);  swch((uint8_t)h); break;
    case '!':    Serial.print("reset-accum");    acc_reset(); break;
    case '?':    Serial.print("accum");    acc_show(); break;
    case 'q':    Serial.print("adc-registers");    adc_show(chc); break;
  }
}

void swch(uint8_t a)
{
  digitalWrite(swzero,(a==swzero)?1:0);  
  digitalWrite(swfull,(a==swfull)?1:0);  
}

void loop()
{
  adc.reset();
  Serial.print(":");
  if (measurev)aloop(chc);
  Serial.print(":");
  iloop();
  Serial.print(":");
//  if (reg) trloop();
  Serial.print(":");
  if(zsc && (sys&1))cloop(chc,swzero,AD7714::MODE_ZERO_SCALE_CAL,AD7714::REG_OFFSET);
  Serial.print(":");
  if(zsc && (sys&2))cloop(chc,swzero,AD7714::MODE_SELF_ZS_CAL,AD7714::REG_OFFSET);
  Serial.print(":");
  if(fsc && (sys&1))cloop(chc,swfull,AD7714::MODE_FULL_SCALE_CAL,AD7714::REG_GAIN);
  Serial.print(":");
  if(fsc && (sys&2))cloop(chc,swfull,AD7714::MODE_SELF_FS_CAL,AD7714::REG_GAIN);
  Serial.println(":>");
  if ((!fsc) && (!zsc) && (!measurev)) delay(300);
}

