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

enum { clko=9 };

AD7714 adc(2.5);
int   chc, flt, pol, gain, mode, ccal;
volatile int drdy = 0;

void adc_intr1()
{
  drdy = 1;
  
}

int wait_intr1()
{
  unsigned long time = millis();
  drdy = 0;
  while(millis()-time < 2000)
    if (drdy) { drdy = 0; return 1; }
  return 0;
}

void setup()
{
  Serial.begin(57600);

  chc = AD7714::CHN_12;
  flt = 1250; // 2MHz/128 = 15635Hz. SR=15625/1250=12.5Hz 
  pol = AD7714::UNIPOLAR;
  gain= AD7714::GAIN_1;
  mode= AD7714::MODE_NORMAL;
  ccal= 0;

  adc_clk();
  Serial.println(":start");

  adc.reset();
  attachInterrupt(0, adc_intr1, FALLING); // D2
  
}

void print_hex(uint8_t x)
{
  if (x < 0x10) Serial.print('0');
  Serial.print(x,HEX);
}

void adc_clk()
{
  // PB1, OC1A, D9 pin 2MHz clock, fosc/128=15625Hz
  //       mode 0100 (CTC),top=OCR1A, COM1A mode=1 (toggle), src=001
  TCCR1A = (0b01 << COM1A0)| (0b00 << WGM10); // div 2
  TCCR1B = (0b01 << WGM12) | (0b001 << CS10); // clk/1 (16MHz)
  OCR1A = 4 - 1; // div 4
  pinMode(clko, OUTPUT);
}

void adc_init()
{
  adc.init(chc, pol, gain, flt, AD7714::NOCALIBRATE); 
}

void adc_show(uint8_t ch)
{
    int32_t v;
    uint8_t m;


    v=adc.command(AD7714::REG_CMM|AD7714::REG_RD|ch, 0xFF);
    Serial.print("#{cmm:0x"); print_hex(v);
    
    v=adc.command(AD7714::REG_MODE|AD7714::REG_RD|ch, 0xFF);
    Serial.print(" mode:0x"); print_hex(v);
    
    m=adc.command(AD7714::REG_FILTH|AD7714::REG_RD|ch, 0xFF);
    v=adc.command(AD7714::REG_FILTL|AD7714::REG_RD|ch, 0xFF);
    Serial.print(" filt:0x"); print_hex(m); print_hex(v);

    v = adc.read24(AD7714::REG_GAIN|AD7714::REG_RD|ch);
    Serial.print(" gain:"); Serial.print(v);
    v = adc.read24(AD7714::REG_OFFSET|AD7714::REG_RD|ch);
    Serial.print(" zero:"); Serial.print(v);
    
    
    v = adc.read24(AD7714::REG_DATA|AD7714::REG_RD|ch);
    Serial.print(" data:");
    
    if (m&AD7714::UNIPOLAR) Serial.print(v);
       else         Serial.print(0x800000-v);
    Serial.println("}");
}


void dloop()
{
  uint8_t reg;
  int dly;
  int32_t v;
  
  dly = adc.t9conv(2000000); // clock 2Mhz, delay 9T
  switch(mode) {
  case AD7714::MODE_NORMAL: reg = AD7714::REG_DATA; dly/=9; break;
   //self calibration DRDY - 6T, read MODE - 3T
  case AD7714::MODE_SELF_ZS_CAL:    reg = AD7714::REG_OFFSET; dly-=dly/3; break;
  case AD7714::MODE_SELF_FS_CAL:    reg = AD7714::REG_GAIN; dly-=dly/3; break;
   //system calibration DRDY 4T, read MODE - 3T
  case AD7714::MODE_ZERO_SCALE_CAL: reg = AD7714::REG_OFFSET; dly=dly*4/9; break;
  case AD7714::MODE_FULL_SCALE_CAL: reg = AD7714::REG_GAIN; dly=dly*4/9; break;
  default: return;
  }
  if (mode != AD7714::MODE_NORMAL)
    adc.command(AD7714::REG_MODE|AD7714::REG_WR|chc, mode); // gain=1, no burnout, no fsync
  if (!wait_intr1()) {  Serial.println(":timeout");  return; }
  v = adc.read24(reg|AD7714::REG_RD|chc);
  if (pol != AD7714::UNIPOLAR && mode == AD7714::MODE_NORMAL) v-=0x800000;
  Serial.println(v);
}

void czloop()
{
  uint8_t reg;
  int32_t v;
  
  mode=AD7714::MODE_SELF_ZS_CAL;    reg = AD7714::REG_OFFSET;
  adc.command(AD7714::REG_MODE|AD7714::REG_WR|chc, mode); // gain=1, no burnout, no fsync
  if (!wait_intr1()) {  Serial.println(":timeout");  return; }
  v = adc.read24(reg|AD7714::REG_RD|chc);
  Serial.print(v);

  mode=AD7714::MODE_ZERO_SCALE_CAL; reg = AD7714::REG_OFFSET;
  adc.command(AD7714::REG_MODE|AD7714::REG_WR|chc, mode); // gain=1, no burnout, no fsync
  if (!wait_intr1()) {  Serial.println(":timeout");  return; }
  v = adc.read24(reg|AD7714::REG_RD|chc);
  Serial.print(' '); Serial.println(v);
}

void cfloop()
{
  uint8_t reg;
  int32_t v;
  
  mode=AD7714::MODE_SELF_FS_CAL;    reg = AD7714::REG_GAIN;
  adc.command(AD7714::REG_MODE|AD7714::REG_WR|chc, mode); // gain=1, no burnout, no fsync
  if (!wait_intr1()) {  Serial.println(":timeout");  return; }
  v = adc.read24(reg|AD7714::REG_RD|chc);
  Serial.print(v);

  mode=AD7714::MODE_FULL_SCALE_CAL; reg = AD7714::REG_GAIN;
  adc.command(AD7714::REG_MODE|AD7714::REG_WR|chc, mode); // gain=1, no burnout, no fsync
  if (!wait_intr1()) {  Serial.println(":timeout");  return; }
  v = adc.read24(reg|AD7714::REG_RD|chc);
  Serial.print(' '); Serial.println(v);
}

void iloop(void)
{
  uint32_t h;
  if (!Serial.available()) return;
  switch(Serial.read()){
    case '\n':case '\r':case ' ': return;
    case 'c': chc=Serial.parseInt(); Serial.print(":channel ");    Serial.println(chc); break;
    case 'r': flt=Serial.parseInt(); Serial.print(":filt-ratio "); Serial.println(flt); break;
    case '?':   adc_show(chc); break;
    case 'i':   adc_init();  Serial.println(":init"); break;
    case 'y':   adc.fsync(chc); break;
    case 'a':   adc.self_calibrate(chc); break;
    case 'd': mode = AD7714::MODE_NORMAL; break;
    case 'z': mode = AD7714::MODE_SELF_ZS_CAL; break;
    case 'f': mode = AD7714::MODE_SELF_FS_CAL; break;
    case 'Z': mode = AD7714::MODE_ZERO_SCALE_CAL; break;
    case 'F': mode = AD7714::MODE_FULL_SCALE_CAL; break;
    case 'x': mode = 0xFF; break;
    case 'b': pol = AD7714::BIPOLAR; Serial.println(":bipolar");  break;
    case 'u': pol = AD7714::UNIPOLAR;Serial.println(":unipolar"); break;
    case 'o': h=Serial.parseInt(); Serial.print(":offset ");    Serial.println(h);
      adc.load24(chc|AD7714::REG_OFFSET|AD7714::REG_WR,h); break;
    case 'g': h=Serial.parseInt(); Serial.print(":gain ");    Serial.println(h);
      adc.load24(chc|AD7714::REG_GAIN|AD7714::REG_WR,h); break;
      break;
    case 'N': ccal=0; break;
    case 'O': ccal=1; break;
    case 'G': ccal=2; break;
  }
}


void loop()
{
  adc.reset();
  switch(ccal) {
  case 0:dloop(); break;
  case 1:czloop(); break;
  case 2:cfloop(); break;
  }
  iloop();
}

