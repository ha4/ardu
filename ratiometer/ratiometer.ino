//DEFAULT:
// ZS:1F4000=2048000
// FS:5761AB=5726635, FS-ZS=3678635

#include <AD7714.h>
#include <math.h>

int cprintf(char *, ...);

enum { mclk=9,  drdy=2 };

class average {
  public:
  average() {  clear(); };
  void clear() { N = 0; sx=0; sxx=0; };
  void sample(double x) { N++; sx+=x; x-=sx/N; sxx+=x*x; };
  double avg() { return sx/N; };
  double stddev() { return sqrt(sxx/N); };
  long N;
  double sx;
  double sxx;
};

AD7714 adc(2500.0);

uint16_t   chc, flt, pol, gain, mode; /* AD7714 registers: (c), (r), (b|u) (p) (adzfZF) */

uint32_t   seq; /* measurement sequence protocol */
uint32_t   seq_sz, seq_ptr;
int        seq_sub;
uint32_t   seq_v;

volatile int drdy_flag = 0;
void adc_intr1() { drdy_flag = 1; }


int is_intr()
{
  if (drdy_flag) { drdy_flag = 0; return 1; } else return 0;
}

void adc_init()
{
  adc.init(chc, pol, gain, flt, AD7714::NOCALIBRATE); 
}

void adc_fsync()
{
  adc.fsync(chc);
}

void adc_gain(int g)
{
  if (g>128||g<=1) gain= AD7714::GAIN_1;
  else if (g <= 2) gain= AD7714::GAIN_2;
  else if (g <= 4) gain= AD7714::GAIN_4;
  else if (g <= 8) gain= AD7714::GAIN_8;
  else if (g <=16) gain= AD7714::GAIN_16;
  else if (g <=32) gain= AD7714::GAIN_32;
  else if (g <=64) gain= AD7714::GAIN_64;
  else if (g<=128) gain= AD7714::GAIN_128;
}

int adc_2gain(uint8_t mode)
{
  switch(mode & AD7714::GAIN_128) {
  case AD7714::GAIN_1: default: return 1;
  case AD7714::GAIN_2: return 2;
  case AD7714::GAIN_4: return 4;
  case AD7714::GAIN_8: return 8;
  case AD7714::GAIN_16: return 16;
  case AD7714::GAIN_32: return 32;
  case AD7714::GAIN_64: return 64;
  case AD7714::GAIN_128: return 128;
  }
}

char* adc_2mode(uint8_t mode)
{
  switch(mode & AD7714::MODE_SELF_FS_CAL) {
  case AD7714::MODE_NORMAL: default: return "norm";
  case AD7714::MODE_SELF_CAL: return "auto";
  case AD7714::MODE_ZERO_SCALE_CAL: return "s_zr";
  case AD7714::MODE_FULL_SCALE_CAL: return "s_fs";
  case AD7714::MODE_SYS_OFFSET_CAL: return "szaf";
  case AD7714::MODE_BG_CALIBRATION: return "bg_z";
  case AD7714::MODE_SELF_ZS_CAL: return "a_zr";
  case AD7714::MODE_SELF_FS_CAL: return "a_fs";
  }
}


void adc_show(uint8_t ch)
{
    int32_t v;
    uint8_t m;

    v=adc.command(AD7714::REG_CMM|AD7714::REG_RD|ch, 0xFF);
    cprintf("#ch%d cmm:%2x-%s", (int)ch, (int)v, (v&0x80)?"RDY":"notRDY");
    
    m=adc.command(AD7714::REG_MODE|AD7714::REG_RD|ch, 0xFF);
    cprintf(" mode:%2x,%s,ampl:%d%s%s", (int)m, adc_2mode(m), adc_2gain(m),
        (m&AD7714::BURNOUT)?"burnout":"",(m&AD7714::FSYNC)?"fsync":"");
    
    m=adc.command(AD7714::REG_FILTH|AD7714::REG_RD|ch, 0xFF);
    v=adc.command(AD7714::REG_FILTL|AD7714::REG_RD|ch, 0xFF);
    v+=256*m;
    cprintf(" flt:%4x(%spolar,%dbit,bust%d,clk%s/%d)", (int)v, (m&AD7714::UNIPOLAR)?"uni":"bi",
        (m&AD7714::WL24)?24:16, (m&AD7714::BST)||1,(m&AD7714::CLK_DIS)?"dis":"en", (int)v&0x0fff);

    v = adc.read24(AD7714::REG_OFFSET|AD7714::REG_RD|ch);
    cprintf(" zr:%l",v);
    v = adc.read24(AD7714::REG_GAIN|AD7714::REG_RD|ch);
    cprintf(" fs:%l",v);
    
    v = adc.read24(AD7714::REG_DATA|AD7714::REG_RD|ch);
    cprintf(" adc: %l\n", (m&AD7714::UNIPOLAR)?v:0x800000-v);
}


void seq0()
{
  uint8_t reg;
  
  switch(mode) {
  default:
  case AD7714::MODE_NORMAL: reg = AD7714::REG_DATA; break;
  case AD7714::MODE_SELF_ZS_CAL:    reg = AD7714::REG_OFFSET; break;
  case AD7714::MODE_SELF_FS_CAL:    reg = AD7714::REG_GAIN; break;
  case AD7714::MODE_ZERO_SCALE_CAL: reg = AD7714::REG_OFFSET; break;
  case AD7714::MODE_FULL_SCALE_CAL: reg = AD7714::REG_GAIN; break;
  }

  adc.reset();
  seq_v = adc.read24(reg|AD7714::REG_RD|chc);
}

void seq1()
{
  if (mode != AD7714::MODE_NORMAL)
    adc.command(AD7714::REG_MODE|AD7714::REG_WR|chc, mode|gain); // no burnout, no fsync
}

void seq8()
{
  cprintf("%4fmV %l\n", adc.conv(seq_v), seq_v);
}

void seq_start(uint32_t n)
{
  seq = n;
  seq_ptr = 1;
  seq_sub = 0;
  while(n!=0) { seq_ptr *= 10; n/=10; }
  seq_ptr/=100; /* drop first digit */
  seq_sz = seq_ptr;
  cprintf(":seq:%l,sz%l\n",seq,seq_sz);
}

void seq_loop()
{
  for(;seq_ptr!=0;) {
  switch((seq/seq_ptr)%10) {
    case 0: seq0(); break;
    case 1: seq1(); break;
//    case 2: seq2(); break;
//    case 3: seq3(); break;
//    case 4: seq4(); break;
//    case 5: seq5(); break;
//    case 6: seq6(); break;
//    case 7: seq7(); break;
    case 8: seq8(); break;
    case 9: seq_ptr /= 10; return;
  }
  if(seq_sub==0) seq_ptr /= 10; else seq_sub--;
  }
  if (seq_ptr == 0) { seq_ptr = seq_sz; if (seq_sz==0) return; }
}

void seq_show()
{
  cprintf(":seqence%l %l:%d\n", seq, seq_ptr, seq_sub);
}

void iloop(void)
{
  uint32_t h;
  int i;

  switch(Serial.read()){
    case '\n':case '\r':case ' ': return;
    case 'q': i=Serial.parseInt(); cprintf(":seq %d\n",i); seq_start(i); break;
    case 'Q': seq_show(); break;
    case '?': adc_show(chc); break;
    case 'i': adc_init();  Serial.println(":init"); break;
    case 'y': adc_fsync(); break;
    case 'c': chc=Serial.parseInt(); cprintf(":channel %d\n",chc);break;
    case 'r': flt=Serial.parseInt(); cprintf(":filt-ratio %d\n",flt); break;
    case 'p': gain=Serial.parseInt(); cprintf(":preamp %d\n",gain); adc_gain(gain); break;
    case 'b': pol = AD7714::BIPOLAR;  Serial.println(":bipolar");  break;
    case 'u': pol = AD7714::UNIPOLAR; Serial.println(":unipolar"); break;
    case 'a': Serial.println(":self-cal");
          adc.self_calibrate(chc);
          Serial.println(":calibrated");
          break;
    case 'd': mode = AD7714::MODE_NORMAL; break;
    case 'z': mode = AD7714::MODE_SELF_ZS_CAL; break;
    case 'f': mode = AD7714::MODE_SELF_FS_CAL; break;
    case 'Z': mode = AD7714::MODE_ZERO_SCALE_CAL; break;
    case 'F': mode = AD7714::MODE_FULL_SCALE_CAL; break;
    case 'o': h=Serial.parseInt(); cprintf(":offset %l\n",h);
              adc.load24(chc|AD7714::REG_OFFSET|AD7714::REG_WR,h); break;
    case 'g': h=Serial.parseInt(); cprintf(":gain %l\n",h);
              adc.load24(chc|AD7714::REG_GAIN|AD7714::REG_WR,h); break;
  }
}

void loop()
{
  if (is_intr()) seq_loop(); 
  if (Serial.available()) iloop();
}


void setup()
{
  Serial.begin(57600);

  chc = AD7714::CHN_12;
  flt = 1250; // 2MHz/128 = 15625Hz. SR=15625/1250=12.5Hz SR=15625/3125=5Hz 
  pol = AD7714::UNIPOLAR; // filterhi - unipolar(1):w24(1):boost(1):clkdis(1):flth(4)
  gain= AD7714::GAIN_1;
  mode= AD7714::MODE_NORMAL; // mode(3):gain(3):burnout(1):fsync(1)

  // PB1, OC1A, D9 pin 2MHz clock, fosc/128=15625Hz
  //       mode 0100 (CTC),top=OCR1A, COM1A mode=1 (toggle), src=001
  TCCR1A = (0b01 << COM1A0)| (0b00 << WGM10); // div 2
  TCCR1B = (0b01 << WGM12) | (0b001 << CS10); // clk/1 (16MHz)
  OCR1A = 4 - 1; // div 4
  pinMode(mclk, OUTPUT);

  pinMode(drdy, INPUT_PULLUP  );

  Serial.println(":start");
  seq_start(1018);

  adc.reset();
  adc_init();

  attachInterrupt(0, adc_intr1, FALLING); // D2
}

