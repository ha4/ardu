#include <GRBlib.h>
#include <IRremote.h>
#include <PinChangeInt.h>

#include "IRremote_rc5.h"
// pwm pins: 3 5 6 9 10 11

rgbled L1(3, 5, 6);
rgbled L2(10,9,11);
rgbled L3(12,8,13,1);

int h[3],ss[3],LL[3],glow[3],ch;
decode_results results;


void setup()
{

  Serial.begin(9600);
  enable_irrecv(2);
  attachPinChangeInterrupt(irparams.recvpin, irisr, CHANGE);
  ch=0;
  h[0]=0;   LL[0]=100; ss[0]=100; glow[0]=0;
  h[1]=120; LL[1]=100; ss[1]=100; glow[1]=0;
  h[2]=240; LL[2]=100; ss[2]=100; glow[2]=0;
}

void putf(char *s,...)
{
         char buf[50];
         va_list x;
         va_start(x,s);
         vsprintf(buf,s,x);
         va_end(x);
         Serial.print(buf);
}

void printraw()
{
          putf("%3d: %04x ",irparams.rawlen, irparams.rawbuf[0]);
          for (int i=1;i<irparams.rawlen;i++)
            putf("%02x%c",irparams.rawbuf[i], ((i+1)==irparams.rawlen)?'\n':' ');
}


void loop()
{
	if (irparams.rcvstate == STATE_STOP) {
          printraw();
	  results.rawbuf   = irparams.rawbuf;
	  results.rawlen   = irparams.rawlen;
	  results.overflow = irparams.overflow;
    	  if (decodeRC5_irrecv(&results)) {
            int b;
            putf("RC5 %dbits: 0x%04x\n", results.bits, results.value);
            b= (results.value&255);
            if (b>=0 && b <=10) h[ch] = b*30;
            if (b==0x37) ch=0;
            if (b==0x36) ch=1;
            if (b==0x32) ch=2;
            if (b==0x34)  glow[ch]=!glow[ch];
            if (b==0x11) LL[ch]-=(LL[ch]<=10)?1:10; if(LL[ch]<0) LL[ch]=0; 
            if (b==0x10) LL[ch]+=(LL[ch]<10)?1:10; if (LL[ch]>100) LL[ch]=100;
            if (b==0x21) ss[ch]-=10; if (ss[ch]<0) ss[ch]=0; 
            if (b==0x20) ss[ch]+=10; if (ss[ch]>100) ss[ch]=100;
            putf("hsv%d,%d,%d",h[ch],ss[ch],LL[ch]);
          }
          resume_irrecv(); // Receive the next value
        }
        L1.hsv(h[0],ss[0],LL[0]);
        L2.hsv(h[1],ss[1],LL[1]);
        L3.hsv(h[2],ss[2],LL[2]);
        if (glow[0]) h[0]++; if (h[0]>360) h[0]-=360;
        if (glow[1]) h[1]++; if (h[1]>360) h[1]-=360;
        if (glow[2]) h[2]++; if (h[2]>360) h[2]-=360;
        delay(50);
        irisr();
}


