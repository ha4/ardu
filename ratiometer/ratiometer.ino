//DEFAULT:
// ZS:1F4000=2048000
// FS:5761AB=5726635, FS-ZS=3678635

#include <SPI.h>
#include "AD7714reg.h"
#include <math.h>
#include "fmt.h"

enum { mclk=9,  drdy=2, cs=10, VRef=2500 };

class average {
public:
  average() { clear(); };
  void clear() { N = 0; sx=0; sxx=0; };
  void sample(double x) { N++; sx+=x; x-=sx/N; sxx+=x*x; };
  double avg() { return sx/N; };
  double stddev() { return sqrt(sxx/N); };
  long N;
  double sx;
  double sxx;
};

uint16_t   chc, flth, flt, pol, gain, mode; /* AD7714 registers: (c), (r), (b|u) (p) (adzfZF) */

volatile int drdy_flag = 0;
void adc_intr1() { 
  drdy_flag = 1; 
}

uint8_t adc_command(uint8_t reg, uint8_t cmd)
{
  uint8_t r;

  digitalWrite(cs,0);
  SPI.transfer(reg);
  r = SPI.transfer(cmd);
  digitalWrite(cs,1);
  return r;
}
void adc_wait(uint8_t channel) 
{
  uint8_t b1;
  digitalWrite(cs,0);
  do {
        SPI.transfer(REG_CMM | channel | REG_RD);
        b1 = SPI.transfer(0xff);
  } while (b1 & REG_nDRDY);
  digitalWrite(cs,1);
}

void adc_reset()
{ 
  digitalWrite(cs,0);
  for (int i = 0; i < 8; i++) SPI.transfer(0xff);
  digitalWrite(cs,1);
}

uint32_t adc_read24(uint8_t reg)
{
  uint32_t r;
  digitalWrite(cs,0);
  SPI.transfer(reg);
    r = SPI.transfer(0xff);
    r = (r << 8) | SPI.transfer(0xff);
    r = (r << 8) | SPI.transfer(0xff);
  digitalWrite(cs,1);

  return r;
}

void adc_load24(uint8_t reg, uint32_t data)
{
  digitalWrite(cs,0);
  SPI.transfer(reg);
  SPI.transfer(0xff & (data >>16));
  SPI.transfer(0xff & (data >>8));
  SPI.transfer(0xff &  data);
  digitalWrite(cs,1);
}

void adc_self_calibrate(uint8_t channel)
{
    adc_command(REG_MODE|channel|REG_WR,  gain | MODE_SELF_CAL);
}

void adc_init()
{
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE3);
  SPI.setClockDivider(SPI_CLOCK_DIV16);

  pinMode(cs,OUTPUT);
  digitalWrite(cs,1);
  adc_reset();
}

void adc_mode()
{
  flth = pol | WL24 | ((flt>>8)& FILTH_MASK); // clkdis=0, bust=0
  adc_command(REG_FILTH|chc|REG_WR,  flth);
  adc_command(REG_FILTL|chc|REG_WR,  flt & FILTL_MASK);
  adc_command(REG_MODE|chc|REG_WR, mode|gain); // fsync=0, burnout=0
}

void adc_fsync()
{
  adc_command(REG_MODE|chc|REG_WR,  mode | gain | FSYNC);
  adc_command(REG_MODE|chc|REG_WR,  (mode |gain )& (~FSYNC));
}

void adc_gain(int g)
{
  if (g>128||g<=1) gain= GAIN_1;
  else if (g <= 2) gain= GAIN_2;
  else if (g <= 4) gain= GAIN_4;
  else if (g <= 8) gain= GAIN_8;
  else if (g <=16) gain= GAIN_16;
  else if (g <=32) gain= GAIN_32;
  else if (g <=64) gain= GAIN_64;
  else if (g<=128) gain= GAIN_128;
}

int adc_2gain(uint8_t mode)
{
  switch(mode & GAIN_128) {
    case GAIN_1:   default:    return 1;
    case GAIN_2:     return 2;
    case GAIN_4:     return 4;
    case GAIN_8:     return 8;
    case GAIN_16:    return 16;
    case GAIN_32:    return 32;
    case GAIN_64:    return 64;
    case GAIN_128:   return 128;
  }
}

char* adc_2mode(uint8_t mode)
{
  switch(mode & MODE_SELF_FS_CAL) {
    case MODE_NORMAL:   default:     return "norm";
    case MODE_SELF_CAL: return "auto";
    case MODE_ZERO_SCALE_CAL:    return "s_zr";
    case MODE_FULL_SCALE_CAL:    return "s_fs";
    case MODE_SYS_OFFSET_CAL:    return "szaf";
    case MODE_BG_CALIBRATION:    return "bg_z";
    case MODE_SELF_ZS_CAL:    return "a_zr";
    case MODE_SELF_FS_CAL:    return "a_fs";
  }
}

float adc_conv(uint32_t code)
{
    uint8_t d = 1 << ((gain & GAIN_128)>>2);
    float sc, ba;
    if (flth & WL24)
	sc=16777216.0;
    else 
	sc=65536.0;
    if (flth & UNIPOLAR)
	ba=0;
    else {
	sc/=2.0; ba=1.0;
    }
    return (code / sc - ba) * VRef / (float)d;
}


void adc_show(uint8_t ch)
{
  int32_t v;
  uint8_t m;

  v=adc_command(REG_CMM|REG_RD|ch, 0xFF);
  cprintf("#ch%d cmm:%02uX-%s", (int)ch, (int)v, (v&0x80)?"READY":"noRDY");

  m=adc_command(REG_MODE|REG_RD|ch, 0xFF);
  cprintf(" mode:%02uX,%s,ampl:%d%s%s", (int)m, adc_2mode(m), adc_2gain(m),
  (m&BURNOUT)?"burnout":"",(m&FSYNC)?"fsync":"");

  m=adc_command(REG_FILTH|REG_RD|ch, 0xFF);
  v=adc_command(REG_FILTL|REG_RD|ch, 0xFF);
  v+=256*m;
  cprintf(" flt:%04uX(%spolar,%dbit,bust%d,clk%s/%d)", (int)v, (m&UNIPOLAR)?"uni":"bi",
  (m&WL24)?24:16, (m&BST)&&1,(m&CLK_DIS)?"dis":"en", (int)v&0x0fff);

  v = adc_read24(REG_OFFSET|REG_RD|ch);
  cprintf(" zr:%06lx",v);
  v = adc_read24(REG_GAIN|REG_RD|ch);
  cprintf(" fs:%06lx",v);

  v = adc_read24(REG_DATA|REG_RD|ch);
  cprintf(" adc: %+07lX\n", (m&UNIPOLAR)?v:0x800000-v);
}

void iloop(void)
{
  uint32_t h;
  int i;

  switch(Serial.read()){
  case '\n':  case '\r':  case ' ':    return;
  case '?':    adc_show(chc);    break;
  case '!':    adc_mode(); Serial.println('!'); break;
  case 'i':    adc_init();    Serial.println(":init");    break;
  case 'y':    adc_fsync();   break;
  case 'c':    chc=Serial.parseInt();    cprintf(":channel %d\n",chc);   break;
  case 'r':    flt=Serial.parseInt();    cprintf(":filt-ratio %d\n",flt);    break;
  case 'p':    gain=Serial.parseInt();   cprintf(":preamp %d\n",gain);   adc_gain(gain);  break;
  case 'b':    pol = BIPOLAR;    Serial.println(":bipolar");    break;
  case 'u':    pol = UNIPOLAR;   Serial.println(":unipolar");   break;
  case 'a':    Serial.println(":self-cal");  adc_self_calibrate(chc); break;
  case 'x':    mode = 0xFF; break;
  case 'd':    mode = MODE_NORMAL;    break;
  case 'z':    mode = MODE_SELF_ZS_CAL;    break;
  case 'f':    mode = MODE_SELF_FS_CAL;    break;
  case 'Z':    mode = MODE_ZERO_SCALE_CAL; break;
  case 'F':    mode = MODE_FULL_SCALE_CAL; break;
  case 'o':    h=Serial.parseInt();    cprintf(":offset %l\n",h);    adc_load24(chc|REG_OFFSET|REG_WR,h); break;
  case 'g':    h=Serial.parseInt();    cprintf(":gain %l\n",h);    adc_load24(chc|REG_GAIN|REG_WR,h);  break;
  }
}

void loop()
{
  uint32_t   seq_v;

  if (drdy_flag) { drdy_flag = 0; 
    uint8_t reg;

    switch(mode) {
    case 0xFF: goto outmode;
    default:
    case MODE_NORMAL:    reg = REG_DATA;     break;
    case MODE_SELF_ZS_CAL: reg = REG_OFFSET; break;
    case MODE_SELF_FS_CAL: reg = REG_GAIN;   break;
    case MODE_ZERO_SCALE_CAL:  reg = REG_OFFSET; break;
    case MODE_FULL_SCALE_CAL:  reg = REG_GAIN;   break;
    }

    seq_v = adc_read24(reg|REG_RD|chc);

    if (mode != MODE_NORMAL)
      adc_command(REG_MODE|REG_WR|chc, mode|gain); // no burnout, no fsync

    cprintf("%4.6fmV %ld\n", adc_conv(seq_v), seq_v);
    outmode:;
  }

  while (Serial.available()) iloop();
}


void setup()
{
  Serial.begin(57600);

  chc = CHN_12;
  flt = 1250; // 2MHz/128 = 15625Hz. SR=15625/1250=12.5Hz SR=15625/3125=5Hz 
  pol = UNIPOLAR; // filterhi - unipolar(1):w24(1):boost(1):clkdis(1):flth(4)
  gain= GAIN_1;
  mode= 0xFF; // mode(3):gain(3):burnout(1):fsync(1)

  // PB1, OC1A, D9 pin 2MHz clock, fosc/128=15625Hz
  //       mode 0100 (CTC),top=OCR1A, COM1A mode=1 (toggle), src=001
  TCCR1A = (0b01 << COM1A0)| (0b00 << WGM10); // div 2
  TCCR1B = (0b01 << WGM12) | (0b001 << CS10); // clk/1 (16MHz)
  OCR1A = 4 - 1; // div 4
  pinMode(mclk, OUTPUT);

  pinMode(drdy, INPUT_PULLUP);

  adc_init();

  attachInterrupt(0, adc_intr1, FALLING); // D2
  Serial.println(":start");
}


