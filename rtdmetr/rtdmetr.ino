#include <SPI.h>
#include "owire.h"

OneWire  ds(14); // on D14

enum { chipSelectPinADC = 9, // mcp3201 #7clk<-sck=D13, #6Dout->MISO=D12, MOSI-nc=D11, #5CS<-D9
       refSelect = 8, // mosfet
       refOut = 7, amp33 = 5, amp16 = 6, // hef4053,  refout #9=C(Z), amp33 #11=A(X), amp16 #10=B(Y)
       muxA = 4, muxB = 2, muxC = 3, // hef4051, A#11, B#10, C#9. A is lsb
};

#define UPART (1.0/4096)
#define UREF 4318.7
#define UADC 1.4265
#define KADC 1.00574466
#define UOFS0 774.48
#define UOFS1 655.22
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

uint32_t tmr1000, tmr100;

double channels[CHANNELS_SZ];
int8_t channel_num;

bool filt;
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

uint16_t read_ds18s20()
{
	byte data[12];
	uint16_t rc = 0xFFFF;

	if (ds.reset()) {
		ds.write(0xCC);     // skip rom
		ds.write(0xBE);         // Read Scratchpad
		ds.read(data, 9);
		if (ds.crc(data, 9)==0)
			rc = data[0] + 256*data[1];
		// restart conversion one second
		ds.reset();
		ds.write(0xCC);     // skip rom
		ds.write(0x44);     // start conversion
		ds.power(true);
	}

	return rc;
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
        channels[ch_mv] = (channels[ch_code] * UREF * UPART + UADC)*KADC;
        if (channels[ch_coeff] == 0)
          channels[channel_num] = channels[ch_mv];
          else 
          channels[channel_num] = (channels[ch_mv]-channels[ch_offs])/channels[ch_amp]*channels[ch_coeff];
        
        if (filt) channels[channel_num] = f_flt(channels[channel_num]);
        channels[ch_tref] = (channels[5] + UDIODE)*KDIODE;
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
  case 'f': filt = !filt; if (filt) f_clr(); break;
  }
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

void setup()
{
	Serial.begin(115200);
	filt = 0;
        for(int i=0; i < CHANNELS_SZ; i++) channels[i]=0;
	SPI.begin();
	SPI.setBitOrder(MSBFIRST);
	SPI.setClockDivider(SPI_CLOCK_DIV16);
	pinMode(chipSelectPinADC, OUTPUT);
	digitalWrite(chipSelectPinADC, HIGH);
	pinMode(refSelect, OUTPUT);
	pinMode(refOut, OUTPUT);
	pinMode(amp16, OUTPUT);
	pinMode(amp33, OUTPUT);
	pinMode(muxA, OUTPUT);
	pinMode(muxB, OUTPUT);
	pinMode(muxC, OUTPUT);
        f_clr();
        uref(0); channels[ch_offs] = UOFS0;
        refSupply(0);
        ampSet(0); channels[ch_amp]=UAMP0;
        muxSet(4); channel_num=4; channels[ch_coeff]=KMUX4;
        tmr1000=0;
        tmr100=0;
}

void loop()
{
        uint32_t ms = millis();

        if (ms - tmr100 >= 100) {
          tmr100 = ms;
          double mv = 0;
	  uint16_t result;
	  for(byte n=8; n--;) {
		result=read_mcp3201();
		mv += result;
	  }
	  channels[ch_code] = mv*0.125; // div 8
          channels[ch_code] = f_flt(channels[ch_code]);

          if (ms - tmr1000 >= 1000) {
            tmr1000 = ms;
            result = read_ds18s20();
            if (result!=0xFFFF && result!=85*16)
              channels[ch_ext] = result*0.0625; // div 16
            f_clr();
            make_report();
            report();
          }

        }

        if (Serial.available()) serial_process();
}
