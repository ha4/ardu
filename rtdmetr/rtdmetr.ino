#include <SPI.h>
#include "owire.h"
#include "phase.h"
#include "mux3201.h"
#include "rtdmeter.h"
#include "linalg.h"

OneWire  ds(16); // on D14

#define UPART (1.0/4096)
#define UREF 4318.7
#define UADC 1.4265
#define KADC 1.00574466
#define IREF 0.2947 /* 0.3ma */
#define UINO (-0.0083) /* -11.7 input offset */
#define UOFS0 777.83 /* old: 774.48 */
#define UOFS1 658.03 /* old: 655.22 */
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


enum { 
  ch_code = 8, ch_mv, ch_offs, ch_amp, ch_coeff, ch_err, ch_ext, ch_tref, CHANNELS_SZ };

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
  if (f_num < 0) { 
    f_num=1; 
    f_sum=x; 
    f_err=0; 
  }
  else { 
    f_num++; 
    f_sum+=x; 
  }
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
  channels[ch_amp] = (mode&BV_A50)?((mode&BV_A33)?UAMP3:UAMP2):
  ((mode&BV_A33)?UAMP1:UAMP0);
  channel_num=muxGet(mode);
  channels[ch_coeff]=(channel_num<4) ?
  ((channel_num<2) ? ((channel_num==0)?KMUX0:KMUX1) : 
  ((channel_num==2)?KMUX2:KMUX3)) :
  ((channel_num<6) ? ((channel_num==4)?KMUX4:KMUX5) : 
  ((channel_num==6)?KMUX6:KMUX7)) ;
  return mode;
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
  } 
  else
    dscnt = 0;

  return rc;
}

void ds18s20_process()
{
  uint16_t result = read_ds18s20();
  if (result!=0xFFFF && result!=85*16)
    channels[ch_ext] = result*0.0625; // div 16
}

uint8_t muxlist[16] = {
  4,5,255};

void acquire_process()
{
  uint32_t ms = millis();

  if (ms - tmr20 < 20) return;
  tmr20 = ms;

  for(int i=0; muxlist[i] != 255 && i < sizeof(muxlist); i++) {
    double r;
    muxSet(muxlist[i]);
    delay(5);
    muxvalues(muxmode(BV_READ));
    channels[ch_code] = 0;
    for(byte n=acq_n; n--;) channels[ch_code] += read_mcp3201();
    channels[ch_code] /= acq_n;
    channels[ch_mv] = (channels[ch_code] * UREF * UPART + UADC)*KADC;
    if (channels[ch_coeff] == 0)
      r = channels[ch_mv];
    else 
      r = ((channels[ch_mv]-channels[ch_offs])/channels[ch_amp]-UINO)*channels[ch_coeff];
    if (aflt) channels[channel_num] = a_flt(channels[channel_num], r, 0.1); 
    else channels[channel_num] = r;        
  }

  if (mflt) channels[channel_num] = f_flt(channels[channel_num]);
  channels[ch_tref] = (channels[5] + UDIODE)*KDIODE;

  if (ms - tmr1000 >= 1000) {
    tmr1000 = ms;
    //f_clr();
    ds18s20_process();
  }

  if (ms-tmrd >= acq_d) { 
    tmrd=ms; 
    report(); 
  }
}

void calibz(int smp)
{
  float cz[2][5][2];
  muxSet(4);
  for(int u=0; u < 2; u++) {
    uref(u);
    delay(2000);
    for(int a=0; a < 4; a++) {
      ampSet(a);
      muxvalues(muxmode(BV_READ));
      delay(100);
      channels[ch_code] = 0;
      for(int n=smp; n--;) channels[ch_code] += read_mcp3201();
      channels[ch_code] /= smp;
      channels[ch_mv] = (channels[ch_code] * UREF * UPART + UADC)*KADC;
      cz[u][a+1][0]=channels[ch_amp];
      cz[u][a+1][1]=channels[ch_mv];
      Serial.print(channels[ch_offs]);
      Serial.print('\t');
      Serial.print(channels[ch_amp]);
      Serial.print('\t');
      Serial.print(channels[ch_mv]);
      Serial.println();
    }
  }
  for(int u=0; u < 2; u++) {
    float r=fit(cz[u][0],4);
    Serial.print("r2=");
    Serial.print(r);
    Serial.print(" a="); Serial.print(cz[u][0][0],6);
    Serial.print(" b="); Serial.print(cz[u][0][1],6);
    Serial.println();
  }
  Serial.print("mean_a="); Serial.println((cz[0][0][0]+cz[1][0][0])*0.5,6);
}


void serial_process()
{
  switch(Serial.read()) {
  case 'R': 
    uref(1); 
    muxvalues(muxmode(BV_READ)); 
    break;
  case 'r': 
    uref(0); 
    muxvalues(muxmode(BV_READ)); 
    break;
  case 'O': 
    refSupply(1); 
    muxvalues(muxmode(BV_READ)); 
    break;
  case 'o': 
    refSupply(0); 
    muxvalues(muxmode(BV_READ)); 
    break;
  case 'a':
    delay(1);
    switch(Serial.read())
    {
    case '0': 
      ampSet(0); 
      break;
    case '1': 
      ampSet(1); 
      break;
    case '2': 
      ampSet(2); 
      break;
    case '3': 
      ampSet(3); 
      break;
    }
    muxvalues(muxmode(BV_READ));
    break;
  case '0': 
    muxlist[0]=0; 
    goto mux_fin;
  case '1': 
    muxlist[0]=1; 
    goto mux_fin;
  case '2': 
    muxlist[0]=2; 
    goto mux_fin;
  case '3': 
    muxlist[0]=3; 
    goto mux_fin;
  case '4': 
    muxlist[0]=4; 
    goto mux_fin;
  case '5': 
    muxlist[0]=5; 
    goto mux_fin;
  case '6': 
    muxlist[0]=6; 
    goto mux_fin;
  case '7': 
    muxlist[0]=7;
mux_fin:
    muxlist[1]=255;
    break;
  case '?': 
    Serial.print("mod "); 
    Serial.println(muxmode(BV_READ),HEX); 
    break;
  case '!': 
    Serial.print("smod "); 
    Serial.println(muxvalues(muxmode(Serial.parseInt())),HEX); 
    break;

  case 'f': 
    mflt = !mflt; 
    if (mflt) f_clr(); 
    Serial.print("meanfilter o"); 
    Serial.println(mflt?"n":"ff");  
    break;
  case 'F': 
    aflt = !aflt; 
    Serial.print("alphafilter o"); 
    Serial.println(aflt?"n":"ff");  
    break;
  case 'p': 
    rept = !rept; 
    Serial.print("report o"); 
    Serial.println(rept?"n":"ff");  
    break;

  case 'm': 
    set_acphase(Serial.parseInt()); 
    break;

  case 'x': 
    Serial.println(read_mcp3201()); 
    break;
  case 'n': 
    delay(1); 
    acq_n=Serial.parseInt(); 
    Serial.print("acq_n "); 
    Serial.println(acq_n); 
    break;
  case 'd': 
    delay(1); 
    acq_d=Serial.parseInt(); 
    Serial.print("acq_delta "); 
    Serial.println(acq_d); 
    break;
  case 'C': 
    delay(1);
    int q;
    for(int i=0; i < sizeof(muxlist); i++) {
      q=Serial.parseInt();
      if ((muxlist[i]=q) == 255) break;
    }
    break;
  case 'z': 
    Serial.println("zero_calib");
    calibz(30000);
    break;
  case 'h': 
    Serial.println("cmd: [0-7]RrOoa[0-3]fFpm[pow]?![mod]xhn[num]d[ms]C[]z"); 
    break;
  }
}

void setup()
{
  Serial.begin(115200);
  init_acphase();
  init_mcp3201();
  init_mux();
  for(int i=0; i < CHANNELS_SZ; i++) channels[i]=0;
  muxvalues(muxmode(BV_READ));
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
  if (rept) acquire_process();
  if (Serial.available()) serial_process();
}



