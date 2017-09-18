//DEFAULT:
// ZS:1F4000=2048000
// FS:5761AB=5726635


#include <AD7714.h>
#include <math.h>
#include "AD7714Test.h"

enum { clko=9 };

int   tmout = 2000; /* command: (t) */
int   chc, flt, pol, gain, mode; /* AD7714 registers: (c), (r), (b|u) () (adzfZFx) */
int   ccal; /* cycle calibration (N)0:no,read data (O)1:zero-cal (G)2:gain-cal */
volatile int drdy = 0;
void adc_clk();
void adc_intr1();
int cprintf(char *, ...);
AD7714 adc(2.5);
average sys, slf; /* kz=ZSsys-ZSself kg=FSsys/FSself kg=1+kg' */
average ts2;
uint32_t tsample=0;

byte timeout(uint32_t *tm, uint32_t now, unsigned int tmout)
{
  if (now - *tm >= tmout) { 
      *tm = now;
      return 1; 
  } else
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

void adc_intr1()
{
  drdy = 1;
}

int wait_intr1()
{
  unsigned long time = millis();
  drdy = 0;
  while(millis()-time < tmout)
    if (drdy) { drdy = 0; return 1; }
  return 0;
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

void adc_gain(int g)
{
  switch(g) {
  case   1: default:  gain= AD7714::GAIN_1; break;
  case   2: gain= AD7714::GAIN_2; break;
  case   4: gain= AD7714::GAIN_4; break;
  case   8: gain= AD7714::GAIN_8; break;
  case  16: gain= AD7714::GAIN_16; break;
  case  32: gain= AD7714::GAIN_32; break;
  case  64: gain= AD7714::GAIN_64; break;
  case 128: gain= AD7714::GAIN_128; break;
  }
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
    cprintf(" gain: %l",v);
    v = adc.read24(AD7714::REG_OFFSET|AD7714::REG_RD|ch);
    cprintf(" zero: %l",v);
    
    
    v = adc.read24(AD7714::REG_DATA|AD7714::REG_RD|ch);
    cprintf(" data: %l}\n", (m&AD7714::UNIPOLAR)?v:0x800000-v);
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
    adc.command(AD7714::REG_MODE|AD7714::REG_WR|chc, mode|gain); // no burnout, no fsync
  if (!wait_intr1()) {  Serial.println(":timeout");  return; }

  v = adc.read24(reg|AD7714::REG_RD|chc);
  cprintf("%4fmV %l\n", 1000.0*adc.conv(v), v);
}

void czloop()
{
  uint8_t reg;
  int32_t v;
  double sy,se;
  
  mode=AD7714::MODE_SELF_ZS_CAL;    reg = AD7714::REG_OFFSET;
  adc.command(AD7714::REG_MODE|AD7714::REG_WR|chc, mode|gain); // no burnout, no fsync
  if (!wait_intr1()) {  Serial.println(":timeout");  return; }
  v = adc.read24(reg|AD7714::REG_RD|chc);
  slf.sample(v);
  se=slf.avg();

  mode=AD7714::MODE_ZERO_SCALE_CAL; reg = AD7714::REG_OFFSET;
  adc.command(AD7714::REG_MODE|AD7714::REG_WR|chc, mode|gain); // no burnout, no fsync
  if (!wait_intr1()) {  Serial.println(":timeout");  return; }
  v = adc.read24(reg|AD7714::REG_RD|chc);
  sys.sample(v);
  sy=sys.avg();
  cprintf("%l %f +- %f    %f +- %f KZ=%f\n", slf.N,se,slf.stddev(),sy,sys.stddev(),sy-se);
 /* SYS = KZ + SELF */
}

void cfloop()
{
  uint8_t reg;
  int32_t v;
  double sy,se;
  
  mode=AD7714::MODE_SELF_FS_CAL;    reg = AD7714::REG_GAIN;
  adc.command(AD7714::REG_MODE|AD7714::REG_WR|chc, mode|gain); // no burnout, no fsync
  if (!wait_intr1()) {  Serial.println(":timeout");  return; }
  v = adc.read24(reg|AD7714::REG_RD|chc);
  slf.sample(v);
  se=slf.avg();

  mode=AD7714::MODE_FULL_SCALE_CAL; reg = AD7714::REG_GAIN;
  adc.command(AD7714::REG_MODE|AD7714::REG_WR|chc, mode|gain); // no burnout, no fsync
  if (!wait_intr1()) {  Serial.println(":timeout");  return; }
  v = adc.read24(reg|AD7714::REG_RD|chc);
  sys.sample(v);
  sy=sys.avg();
  cprintf("%l %f +- %f    %f +- %f KG=%f\n", slf.N,se,slf.stddev(),sy,sys.stddev(),
      (sy/se-1.0)*16777216.0); /* SYS=SELF+SELF*KG/2^24*/
}

void iloop(void)
{
  uint32_t h;
  if (!Serial.available()) return;
  switch(Serial.read()){
    case '\n':case '\r':case ' ': return;
    case 'N': ccal=0; break;
    case 'O': ccal=1; sys.clear(); slf.clear(); break;
    case 'G': ccal=2; sys.clear(); slf.clear(); break;
    case 'T': ccal=3; ts2.clear(); break;
    case 'c': chc=Serial.parseInt(); cprintf(":channel %d\n",chc);break;
    case 'r': flt=Serial.parseInt(); cprintf(":filt-ratio %d\n",flt); break;
    case 'p': gain=Serial.parseInt(); cprintf(":preamp %d\n",gain); adc_gain(gain); break;
    case 't': tmout=Serial.parseInt(); cprintf(":timeout %dms\n", tmout); break;
    case '?': adc_show(chc); break;
    case 'i': adc_init();  Serial.println(":init"); break;
    case 'y': adc.fsync(chc); break;
    case 'a': Serial.println(":self-cal"); adc.self_calibrate(chc); Serial.println(":calibrated");  break;
    case 'd': mode = AD7714::MODE_NORMAL; break;
    case 'z': mode = AD7714::MODE_SELF_ZS_CAL; break;
    case 'f': mode = AD7714::MODE_SELF_FS_CAL; break;
    case 'Z': mode = AD7714::MODE_ZERO_SCALE_CAL; break;
    case 'F': mode = AD7714::MODE_FULL_SCALE_CAL; break;
    case 'x': mode = 0xFF; break;
    case 'b': pol = AD7714::BIPOLAR;  Serial.println(":bipolar");  break;
    case 'u': pol = AD7714::UNIPOLAR; Serial.println(":unipolar"); break;
    case 'o': h=Serial.parseInt(); cprintf(":offset %l\n",h);
              adc.load24(chc|AD7714::REG_OFFSET|AD7714::REG_WR,h); break;
    case 'g': h=Serial.parseInt(); cprintf(":gain %l\n",h);
              adc.load24(chc|AD7714::REG_GAIN|AD7714::REG_WR,h); break;
  }
}

void tloop()
{
    double v;
    int x = 30000+random(50);
    ts2.sample(x);
    cprintf("%d %d %f +- %f\n",x, ts2.N, ts2.avg(),ts2.stddev());
}

void loop()
{
  adc.reset();
  switch(ccal) {
  case 0:dloop(); break;
  case 1:czloop(); break;
  case 2:cfloop(); break;
  }
  if (timeout(&tsample, millis(), tmout) && ccal==3) tloop();
  iloop();
}


