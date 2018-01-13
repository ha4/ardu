#include <SPI.h>
#include "owire.h"
#include "phase.h"
#include "mux3201.h"
#include "rtdmeter.h"

OneWire  ds(16); // on D14

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

double channels[CHANNELS_SZ];
int8_t channel_num;

bool mflt;
bool aflt;
bool rept;
bool seqq;

uint32_t tmr1000, tmr20, tmrd;
int  acq_n;
int  acq_d;

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

uint8_t muxvalues(uint8_t mode)
{
  channels[ch_offs] = (mode&BV_REFS)?UOFS1:UOFS0;
  channels[ch_amp] = (mode&BV_A50)?((mode&BV_A33)?UAMP3:UAMP2):((mode&BV_A33)?UAMP1:UAMP0);
  channel_num=muxGet(mode);
  channels[ch_coeff]=(channel_num<4) ?
    ((channel_num<2) ? ((channel_num==0)?KMUX0:KMUX1) : ((channel_num==2)?KMUX2:KMUX3)) :
    ((channel_num<6) ? ((channel_num==4)?KMUX4:KMUX5) : ((channel_num==6)?KMUX6:KMUX7)) ;
  return mode;
}

void make_report()
{
        double r;
        channels[ch_mv] = (channels[ch_code] * UREF * UPART + UADC)*KADC;
        if (channels[ch_coeff] == 0)
          r = channels[ch_mv];
          else 
          r = ((channels[ch_mv]-channels[ch_offs])/channels[ch_amp]-UINO)*channels[ch_coeff];
        if (aflt) channels[channel_num] = a_flt(channels[channel_num], r, 0.1); else channels[channel_num] = r;
        
        if (mflt) channels[channel_num] = f_flt(channels[channel_num]);
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

void ds18s20_process()
{
   uint16_t result = read_ds18s20();
   if (result!=0xFFFF && result!=85*16)
      channels[ch_ext] = result*0.0625; // div 16
}

static uint8_t mx_lst[16] = { BV_MUX0|BV_REF655|BV_A17, 20,  BV_MUX1|BV_REF655|BV_A17, 20, BV_END };
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
    sprintf(buf,"q%02x %04d", m[n].mode, m[n].result);
    Serial.println(buf);
  }
    
}

void seqence_query()
{
    seq_t m = seq_sample();
    Serial.print('q');
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
	for(byte n=acq_n; n--;) {
		result=read_mcp3201();
		mv += result;
	}
        channels[ch_code] = mv/acq_n;/* div 8 */

        if (ms - tmr1000 >= 1000) {
            tmr1000 = ms;
            //f_clr();
	    ds18s20_process();
        }

        make_report();
        if (ms-tmrd >= acq_d) { tmrd=ms; report(); }
}

void serial_process()
{
  switch(Serial.read()) {
  case 'R': uref(1); muxvalues(muxmode(BV_MUXREAD)); break;
  case 'r': uref(0); muxvalues(muxmode(BV_MUXREAD)); break;
  case 'O': refSupply(1); muxvalues(muxmode(BV_MUXREAD)); break;
  case 'o': refSupply(0); muxvalues(muxmode(BV_MUXREAD)); break;
  case 'a':
    delay(1);
    switch(Serial.read())
    {
    case '0': ampSet(0); break;
    case '1': ampSet(1); break;
    case '2': ampSet(2); break;
    case '3': ampSet(3); break;
    }
    muxvalues(muxmode(BV_MUXREAD));
    break;
  case '0': muxSet(0); muxvalues(muxmode(BV_MUXREAD)); break;
  case '1': muxSet(1); muxvalues(muxmode(BV_MUXREAD)); break;
  case '2': muxSet(2); muxvalues(muxmode(BV_MUXREAD)); break;
  case '3': muxSet(3); muxvalues(muxmode(BV_MUXREAD)); break;
  case '4': muxSet(4); muxvalues(muxmode(BV_MUXREAD)); break;
  case '5': muxSet(5); muxvalues(muxmode(BV_MUXREAD)); break;
  case '6': muxSet(6); muxvalues(muxmode(BV_MUXREAD)); break;
  case '7': muxSet(7); muxvalues(muxmode(BV_MUXREAD)); break;
  case '?': Serial.print("mod "); Serial.println(muxmode(BV_MUXREAD),HEX); break;
  case '!': Serial.print("smod "); Serial.println(muxvalues(muxmode(Serial.parseInt())),HEX); break;

  case 'f': mflt = !mflt; if (mflt) f_clr(); Serial.print("meanfilter o"); Serial.println(mflt?"n":"ff");  break;
  case 'F': aflt = !aflt; Serial.print("alphafilter o"); Serial.println(aflt?"n":"ff");  break;

  case 'm': set_acphase(Serial.parseInt()); break;

  case 'p': rept = !rept; Serial.print("report o"); Serial.println(rept?"n":"ff");  break;
  case 'Q': seqq = !seqq; Serial.print("seqencer o"); Serial.println(seqq?"n":"ff");  break;
  case 'q': get_seqence();  break;
  case 'S': seqence_read(); break;
  case 's': seqence_print(); break;
  case 'x': Serial.println(read_mcp3201()); break;
  case 'n': delay(1); acq_n=Serial.parseInt(); Serial.print("acq_n "); Serial.println(acq_n); break;
  case 'd': delay(1); acq_d=Serial.parseInt(); Serial.print("acq_delta "); Serial.println(acq_d); break;

  case 'h': Serial.println("cmd: [0-7]RrOoa[0-3]fFm[pow]pQq?![mod]S[]sxhn[num]d[ms]"); break;
  }
}

void setup()
{
	Serial.begin(115200);
        init_acphase();
        init_mcp3201();
	init_mux();
        for(int i=0; i < CHANNELS_SZ; i++) channels[i]=0;
        muxvalues(muxmode(BV_MUXREAD));
        rept = 1;
        seqq = 0;
        acq_n=8;
        acq_d=1000;

	mflt = 0;
        aflt = 1;
        f_clr();

        tmr1000=0;
        tmr20=0;
        tmrd=0;
}

void loop()
{
	if (seqq) seqence_query();
	if (rept) acquire_process();
        if (Serial.available()) serial_process();
}

