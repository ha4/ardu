#include <SPI.h>
#include "owire.h"
#include "phase.h"
#include "rtdmeter.h"

OneWire  ds(16); // on D14



enum { chipSelectPinADC = 9, // mcp3201 #7clk<-sck=D13, #6Dout->MISO=D12, MOSI-nc=D11, #5CS<-D9
       refSelect = 8, // mosfet
       refOut = 7, amp33 = 5, amp16 = 6, // hef4053,  refout #9=C(Z), amp33 #11=A(X), amp16 #10=B(Y)
       muxA = 4, muxB = 2, muxC = 3, // hef4051, A#11, B#10, C#9. A is lsb
};

#define BV_MUXA 0x10
#define BV_MUXB 0x04
#define BV_MUXC 0x08
#define BV_MUX0 (0x00)
#define BV_MUX1 (BV_MUXA)
#define BV_MUX2 (BV_MUXB)
#define BV_MUX3 (BV_MUXB|BV_MUXA)
#define BV_MUX4 (BV_MUXC)
#define BV_MUX5 (BV_MUXC|BV_MUXA)
#define BV_MUX6 (BV_MUXC|BV_MUXB)
#define BV_MUX7 (BV_MUXC|BV_MUXB|BV_MUXA)
#define BV_MUX  (BV_MUX7)
#define BV_REFO 0x80
#define BV_AR16  0x40
#define BV_AR33  0x20
#define BV_A17  (0x00)
#define BV_A33  (BV_AR16)
#define BV_A50  (BV_AR33)
#define BV_A66  (BV_AR16|BV_AR33)
#define BV_AMP  (BV_A66)
#define BV_REFS 0x01
#define BV_REF777 0
#define BV_REF655 (BV_REFS)
#define BV_EXT  0xFE
#define BV_END  0xFF
#define BV_MUXREAD 0xFF
#define BV_MASKD 0xFC
#define BV_MASKB 0x01
#define BV_PORTD PORTD
#define BV_PORTB PORTB

#define UPART (1.0/4096)
#define UREF 4318.7
#define UADC 1.4265
#define KADC 1.00574466
#define UINO -0.0117 /* -11.7 input offset */
#define UOFS0 775.08 /* old: 774.48 */
#define UOFS1 655.80 /* old: 655.22 */
#define UAMP0 17.32605  
#define UAMP1 33.17603
#define UAMP2 49.92801
#define UAMP3 65.779736
#define KMUX0 1 /* A rtd */
#define KMUX1 1 /* B tc */
#define KMUX2 101 /* B(v) */
#define KMUX3 5 /* A(mA) */
#define KMUX4 0 /* zero, no amp */
#define KMUX5 15.91445 /* t-diode */
#define KMUX6 1 /* C tc */
#define KMUX7 1 /* D tc */
#define UDIODE -573.57055
#define KDIODE -0.494338


enum { ch_code = 8, ch_mv, ch_offs, ch_amp, ch_coeff, ch_err, ch_ext, ch_tref, CHANNELS_SZ };

uint32_t tmr1000, tmr20;

double channels[CHANNELS_SZ];
int8_t channel_num;

bool filt;
bool rept;
bool seqq;
double f_sum;
int32_t f_num;
double f_err;

void f_clr()
{
  f_num=-1;
}

double f_flt(double x)
{
  double t;
  if (f_num < 0) { f_num=1; f_sum=x; f_err=0; }
  else { f_num++; f_sum+=x; }
  t  = f_sum/f_num;
  f_err += (x - t - f_err)/f_num; /* err = (err*(num-1) + delta)/num */
  channels[ch_err]=f_err;
  return t;
}

double a_flt(double y, double x, double a)
{
  return a*(x-y)+y;
}

uint16_t read_ds18s20()
{
        static uint8_t dscnt=0;
	byte data[12];
	uint16_t rc = 0xFFFF;

	if (ds.reset()) {
                if (dscnt) { // read data only after conversion
  		  ds.write(0xCC);     // skip rom
		  ds.write(0xBE);         // Read Scratchpad
		  ds.read(data, 9);
		  if (ds.crc(data, 9)==0)
			rc = (data[1]<<8) | data[0];
                }
		// restart conversion one second
		ds.reset();
		ds.write(0xCC);     // skip rom
		ds.write(0x44);     // start conversion
		ds.power(true);
                dscnt++;
                if (!dscnt) dscnt++;
	} else
          dscnt = 0;

	return rc;
}

void init_mcp3201()
{
	SPI.begin();
	SPI.setBitOrder(MSBFIRST);
	SPI.setClockDivider(SPI_CLOCK_DIV16); //62.5kS/s, 1mbps.
	pinMode(chipSelectPinADC, OUTPUT);
	digitalWrite(chipSelectPinADC, HIGH);
}

uint16_t read_mcp3201() /* 30us to convert/read */
{
	uint16_t result;

	digitalWrite(chipSelectPinADC, LOW);
	result = SPI.transfer(0x00) << 8;
	result = result | SPI.transfer(0x00);
	digitalWrite(chipSelectPinADC, HIGH);
	result = result >> 1;
	result = result & 0xFFF;
  
	return result;
}

void init_mux()
{
	pinMode(refSelect, OUTPUT);
	pinMode(refOut, OUTPUT);
	pinMode(amp16, OUTPUT);
	pinMode(amp33, OUTPUT);
	pinMode(muxA, OUTPUT);
	pinMode(muxB, OUTPUT);
	pinMode(muxC, OUTPUT);

        for(int i=0; i < CHANNELS_SZ; i++) channels[i]=0;

        uref(0);
        refSupply(0);
        ampSet(0);
        muxSet(4);
}

uint8_t muxmode(uint8_t mode)
{
  uint8_t b,d;

  if (mode==BV_MUXREAD) {
    b = BV_PORTB & BV_MASKB;
    d = BV_PORTD & BV_MASKD;
    return b|d;
  }
  b = mode & BV_MASKB;
  d = mode & BV_MASKD;
  BV_PORTB = (BV_PORTB & ~BV_MASKB) | b;
  BV_PORTD = (BV_PORTD & ~BV_MASKB) | d;
  return mode;
}


uint8_t muxvalues(uint8_t mode)
{
  channels[ch_offs] = (mode&BV_REFS)?UOFS1:UOFS0;
  channels[ch_amp] = (mode&BV_A50)?((mode&BV_A33)?UAMP3:UAMP2):((mode&BV_A33)?UAMP1:UAMP0);
  channel_num=(mode&BV_MUXC?4:0)|(mode&BV_MUXB?2:0)|(mode&BV_MUXA?1:0);
  channels[ch_coeff]=(channel_num<4) ?
    ((channel_num<2) ? ((channel_num==0)?KMUX0:KMUX1) : ((channel_num==2)?KMUX2:KMUX3)) :
    ((channel_num<6) ? ((channel_num==4)?KMUX4:KMUX5) : ((channel_num==6)?KMUX6:KMUX7)) ;
  return mode;
}

/* 0->0.777v 1->0.666v */
void uref(int mode)
{
  digitalWrite(refSelect, mode);
  channels[ch_offs] = (mode==1)?UOFS1:UOFS0;
  Serial.print("Uref ");
  Serial.println(mode);
}

/* 0->off, 1->on: tempr-diode, rtd(300uA), 10M-to-tc supply */
void refSupply(int mode)
{
  digitalWrite(refOut, mode);
  Serial.print("ref out ");
  Serial.println(mode);
}

/* 0->1:(16+16+33) 1->1:(16+33) 2->1:(16+16) 3->1:16 */
void ampSet(int s)
{
  digitalWrite(amp16, (s&1)?1:0);
  digitalWrite(amp33, (s&2)?1:0);
  channels[ch_amp]=(s<2)?((s==0)?UAMP0:UAMP1):((s==2)?UAMP2:UAMP3);
  Serial.print("amp ");
  Serial.println(s);
}

/*   A      <-R           <-mA(+)
 *   B      <-+ <-,   <-V   |
 *   B'=GND <-' <-'tc <-' <-'
 *   C  <-tc-'
 *   D  [as c]
 * channels:
 * 0:A, 1:B, 2:B(v), 3:A(mA), 4:zero, 5:Tmpr-diode, 6:C, 7:D
 */

void muxSet(int s)
{
  digitalWrite(muxA, (s&1)?1:0);
  digitalWrite(muxB, (s&2)?1:0);
  digitalWrite(muxC, (s&4)?1:0);
  channel_num=s&7;
  channels[ch_coeff]=(s<4)?((s<2)?((s==0)?KMUX0:KMUX1):((s==2)?KMUX2:KMUX3)):((s<6)?((s==4)?KMUX4:KMUX5):((s==6)?KMUX6:KMUX7));
  Serial.print("mux ");
  Serial.println(s);
}

void make_report()
{
        double r;
        channels[ch_mv] = (channels[ch_code] * UREF * UPART + UADC)*KADC;
        if (channels[ch_coeff] == 0)
          r = channels[ch_mv];
          else 
          r = ((channels[ch_mv]-channels[ch_offs])/channels[ch_amp]-UINO)*channels[ch_coeff];
        channels[channel_num] = a_flt(channels[channel_num], r, 0.1);
        
        if (filt) channels[channel_num] = f_flt(channels[channel_num]);
        channels[ch_tref] = (channels[5] + UDIODE)*KDIODE;
}

void report()
{
  Serial.print(channel_num);
  Serial.print(' ');
  for(int i = 0; i < 8; i++) {
    Serial.print(channels[i],3);
    Serial.print(' ');
  }
  Serial.print(' ');
  for(int i = 8; i < CHANNELS_SZ; i++) {
    Serial.print(channels[i],3);
    Serial.print(' ');
  }
  Serial.println();
}

void ds18s20_process()
{
   uint16_t result = read_ds18s20();
   if (result!=0xFFFF && result!=85*16)
      channels[ch_ext] = result*0.0625; // div 16
}

static uint8_t mx_lst[16] = { BV_MUX4|BV_REF655|BV_A17, 20,  BV_MUX4|BV_REF777|BV_A17, 20, BV_END };
static uint8_t mx_cnt = 0;
static uint8_t mx_num = 0;

void seqence_print()
{
  Serial.print('S');
  for(int i=0; i < sizeof(mx_lst); i++) {
    Serial.print(mx_lst[i]);
    if (mx_lst[i] == BV_END) {
      Serial.println();
      break;
    }
    else Serial.print(',');
  }
}

void seqence_read()
{
  int q;
  for(int i=0; i < sizeof(mx_lst); i++) {
    q=Serial.parseInt();
    if ((mx_lst[i]=q) == BV_END) break;
  }
}


seq_t seq_sample()
{
  seq_t r;

  r.mode=mx_lst[mx_cnt];
  muxmode(r.mode);
  r.result = read_mcp3201();

  mx_num++;
  if (mx_num >= mx_lst[mx_cnt+1]) {
    mx_num=0;
    mx_cnt+=2;
    if (mx_lst[mx_cnt]==BV_END) mx_cnt=0;
  }

  return r;
}

void get_seqence()
{
  seq_t m[50];
  char buf[20];
  
  for(int n=0;n<40;n++)
    m[n] = seq_sample();

  for(int n=0;n<40;n++) {
    sprintf(buf,"%02x %04d", m[n].mode, m[n].result);
    Serial.println(buf);
  }
    
}

void seqence_query()
{
    seq_t m = seq_sample();
    Serial.print(m.mode, HEX);
    Serial.print(' ');
    Serial.println(m.result);
}


void acquire_process()
{
        uint32_t ms = millis();

        if (ms - tmr20 < 20) return;
        tmr20 = ms;

        double mv = 0;
	uint16_t result;
	for(byte n=8; n--;) {
		result=read_mcp3201();
		mv += result;
	}
        channels[ch_code] = mv*0.125;/* div 8 */

        if (ms - tmr1000 >= 1000) {
            tmr1000 = ms;
            //f_clr();
	    ds18s20_process();
            make_report();
            report();
        } else 
             make_report();
}

void serial_process()
{
  switch(Serial.read()) {
  case 'R': uref(1); break;
  case 'r': uref(0); break;
  case 'O': refSupply(1); break;
  case 'o': refSupply(0); break;
  case 'a':
    delay(1);
    switch(Serial.read())
    {
    case '0': ampSet(0); break;
    case '1': ampSet(1); break;
    case '2': ampSet(2); break;
    case '3': ampSet(3); break;
    }
    break;
  case '0': muxSet(0); break;
  case '1': muxSet(1); break;
  case '2': muxSet(2); break;
  case '3': muxSet(3); break;
  case '4': muxSet(4); break;
  case '5': muxSet(5); break;
  case '6': muxSet(6); break;
  case '7': muxSet(7); break;
  case 'f': filt = !filt; if (filt) f_clr(); Serial.print("filter o"); Serial.println(filt?"n":"ff");  break;
  case 'm': set_acphase(Serial.parseInt()); break;
  case 'p': rept = !rept; Serial.print("report o"); Serial.println(rept?"n":"ff");  break;
  case 'Q': seqq = !seqq; Serial.print("seqencer o"); Serial.println(seqq?"n":"ff");  break;
  case 'q': get_seqence();  break;
  case '?': Serial.print("mod "); Serial.println(muxmode(BV_MUXREAD),HEX); break;
  case '!': Serial.print("smod "); Serial.println(muxvalues(muxmode(Serial.parseInt())),HEX); break;
  case 'S': seqence_read(); break;
  case 's': seqence_print(); break;
  case 'x': Serial.println(read_mcp3201()); break;
  case 'h': Serial.println("cmd: RrOoa[0-3][0-7]fm[int]pQq?![mod]Ssxh"); break;
  }
}

void setup()
{
	Serial.begin(115200);
        init_acphase();
        init_mcp3201();
	init_mux();

        rept = 1;
        seqq = 0;

	filt = 0;
        f_clr();

        tmr1000=0;
        tmr20=0;
}

void loop()
{
	if (seqq) seqence_query();
	if (rept) acquire_process();
        if (Serial.available()) serial_process();
}

