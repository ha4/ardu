#include <GRBlib.h>
#include <PinChangeInt.h>
#include <EEPROM.h>

#include "IRremote_rc5.h"
#include "buttn.h"
// pwm pins: 3 5 6 9 10 11

enum {
  SEQ_POWER, SEQ_PON, SEQ_POFF, // 0,1,2
  SEQ_CH, SEQ_CH1, SEQ_CH2, SEQ_CH3, SEQ_CH4, // 3,4,5,6,7
  SEQ_COLOR, SEQ_R, SEQ_G, SEQ_B, SEQ_BLACK, SEQ_WHITE, SEQ_FASTGLOW, SEQ_ALLGLOW, // 8,9,10,11,12,13,14
  SEQ_GLOW, SEQ_NGLOW, 
  SEQ_SAT, SEQ_NSAT,
  SEQ_LUM, SEQ_NLUM,
  SEQ_SETX,
};

void seq_cmd(byte c);
void seq_move();
void btn_decoder();
void rc5_decoder();
void ee_sav();
void ee_load();

rgbpdm L1(11,12,10); // 3,5,6
rgbpdm L2(9,8,13); // 10,9,11
rgbpdm L3(5,3,6); // 12,8,13
rgbpdm L4(15,16,14); // 14,15,16

button b1(4); // D4

#define DBVERSION 0x1001
#define EE_PARN (5*4+1)
#define CRC_START 0xFFFF
uint16_t crc;

int dbv;
byte seq_mode, seq_xmode;
int h[4],ss[4],LL[4],glow[4],ccc[4];
int gcnt[4];
int ch;
decode_results results;

uint32_t t_20ms;
uint32_t t_1ms;
uint32_t t_1000ms;
uint32_t idle_cnt;
uint32_t idle_prv;

void setup()
{

  Serial.begin(9600);
  enable_irrecv(2);
  attachPinChangeInterrupt(irparams.recvpin, irisr, CHANGE);
  L1.inv = L2.inv = L3.inv = L4.inv = 1;
  ch=0;
  seq_cmd(SEQ_PON);
  t_20ms=0;
  t_1ms=0;
  t2_setup();
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
  uint32_t t=micros(); 
//  idle_cnt++;

 if (t-t_1ms >= 1000) { t_1ms+=1000;  seq_move(); }
 rc5_decoder();
 if (t-t_20ms >= 20000) { t_20ms+=20000;
   btn_decoder();
   irisr();
 }

//  if (t-t_1000ms >= 1000000L) { t_1000ms+=1000000; 
//    t = idle_cnt - idle_prv;
//    idle_prv = idle_cnt;
//    Serial.println(t); // 69min, 110400 max, 109044 toie2, 90714 pdm1, 74820 pdm2, 61310 pdm3, 47748 pdm4
//  }

}

void t2_setup()
{
  // timer mode
  	TIMSK2 &= ~(1<<TOIE2);
	TIMSK2 &= ~(1<<OCIE2A); // OCI disable
	ASSR &= ~(1<<AS2); // clock from CLKio
	TCCR2B &= ~((1<<CS22) | (1<<CS21) | (1<<CS20)); // disable clock
	TCCR2A &= ~((1<<WGM21) | (1<<WGM20)); TCCR2B &= ~(1<<WGM22); // mode 0
	TCCR2A |=   (1<<WGM21) | (0<<WGM20);  TCCR2B |=  (0<<WGM22); // mode 2 CTC, OCRA
	OCR2A = 154; // TOP for mode 2
  // prescaler=8
	TCCR2B |= (0<<CS22) | (1<<CS21) | (0<<CS20);
  // start
//    	TIMSK2 |= (1<<TOIE2);
    	TIMSK2 |= (1<<OCIE2A);
}

//#define ISRt2 TIMER2_OVF_vect
#define ISRt2 TIMER2_COMPA_vect

ISR(ISRt2) { // 77.5us, *255=19.84ms 50.4Hz
  L1.modx();
  L2.modx();
  L3.modx();
  L4.modx();
}

void rc5_decoder()
{
  int b;

  if (irparams.rcvstate != STATE_STOP) return;
  // printraw();
  results.rawbuf   = irparams.rawbuf;
  results.rawlen   = irparams.rawlen;
  results.overflow = irparams.overflow;
  if (!decodeRC5_irrecv(&results))  { resume_irrecv(); return; }

  // putf("RC5 %dbits: 0x%04x\n", results.bits, results.value);
  b= (results.value&255);
  resume_irrecv(); // Receive the next value

  if (b>=0 && b < 10) h[ch] = b*27;
  else switch (b) {
  case 0x37: seq_cmd(SEQ_CH1); break;
  case 0x36: seq_cmd(SEQ_CH2); break;
  case 0x32: seq_cmd(SEQ_CH3); break;
  case 0x34: seq_cmd(SEQ_CH4); break;
  case 0x0C: seq_cmd(SEQ_POFF); break;
  case 0x29: seq_cmd(SEQ_GLOW); break;
  case 0x1E: seq_cmd(SEQ_NGLOW); break;
  case 0x11: seq_cmd(SEQ_NLUM); break;
  case 0x10: seq_cmd(SEQ_LUM); break;
  case 0x21: seq_cmd(SEQ_NSAT); break;
  case 0x20: seq_cmd(SEQ_SAT); break;
  }
}

void btn_decoder() // 20ms poll
{
  byte p;
  if ((p = b1.poll())==BTN_NONE) return;
  
  if (p==BTN_LONG) { seq_cmd(SEQ_POWER); return; }
  if (seq_mode==SEQ_POFF) { seq_cmd(SEQ_PON); return; }
  
  if (seq_mode==SEQ_SETX)
    switch(p) {
    case BTN_CLICK:  
     switch(seq_xmode) {
     case 0: h[ch]+=8; if (h[ch] > 255) h[ch]=0; break;
     case 1: LL[ch]+=8; if (LL[ch] > 255) LL[ch]=0; break;
     case 2: ss[ch]+=8; if (ss[ch] > 255) ss[ch]=0; break;
     case 3: glow[ch]+=2; if (glow[ch] > 55) glow[ch]=0; break;
     }
     break;
    case BTN_DCLICK:  seq_xmode++; if (seq_xmode >= 4) seq_cmd(SEQ_SETX); break;
    case BTN_MCLICK:  seq_cmd(SEQ_SETX); break;
    }
  else 
    switch(p) {
    case BTN_CLICK:   seq_cmd(SEQ_CH); break;
    case BTN_DCLICK:  seq_cmd(SEQ_COLOR); break;
    case BTN_MCLICK:  seq_cmd(SEQ_SETX); break;
    }
}

void flash(byte h, byte s, byte l)
{
  switch(ch) {
  case 0: L1.hsl(h,s,l); break;
  case 1: L2.hsl(h,s,l); break;
  case 2: L3.hsl(h,s,l); break;
  case 3: L4.hsl(h,s,l); break;
  }
}

void seq_cmd(byte c)
{
  seq_lagain:
//  putf("cmd%d\n",(int)c);
  switch(c) {
  case SEQ_CH: ch++; if (ch > 3) ch=0;
   flash(h[ch],0,255);  delay(100);
   flash(h[ch],0,0); delay(200);
   flash(h[ch],ss[ch],LL[ch]);
   break;
  case SEQ_CH1: ch=0; break;
  case SEQ_CH2: ch=1; break;
  case SEQ_CH3: ch=2; break;
  case SEQ_CH4: ch=3; break;
  case SEQ_POFF: seq_mode=SEQ_POFF;
              ee_sav();
              LL[0]=LL[1]=LL[2]=LL[3]=0; ss[0]=ss[1]=ss[2]=ss[3]=0; break;
  case SEQ_PON: seq_mode=SEQ_PON;
              LL[0]=LL[1]=LL[2]=LL[3]=127; ss[0]=ss[1]=ss[2]=ss[3]=255;
              gcnt[0]=gcnt[1]=gcnt[2]=gcnt[3]=0;
              glow[0]=glow[1]=glow[2]=glow[3]=5;
              h[0]=0; h[1]=85; h[2]=170; h[3]=192;
              ee_load();
              break;
  case SEQ_POWER: if (seq_mode==SEQ_POFF) c=SEQ_PON; else c=SEQ_POFF; goto seq_lagain;
  case SEQ_GLOW: glow[ch]=1; break;
  case SEQ_NGLOW: glow[ch]=0; break;
  case SEQ_SAT: ss[ch]+=10; if (ss[ch]>255) ss[ch]=255;  break;
  case SEQ_NSAT: ss[ch]-=10; if (ss[ch]<0) ss[ch]=0;   break;
  case SEQ_LUM: LL[ch]+=(LL[ch]<10)?1:10; if (LL[ch]>255) LL[ch]=255;  break;
  case SEQ_NLUM: LL[ch]-=(LL[ch]<=10)?1:10; if(LL[ch]<0) LL[ch]=0;   break;
  case SEQ_COLOR: ccc[ch]++; if (ccc[ch] > SEQ_ALLGLOW || ccc[ch] < SEQ_R) ccc[ch]=SEQ_R; c=ccc[ch]; goto seq_lagain;
  case SEQ_R: h[ch]=0; LL[ch]=127; ss[ch]=255; glow[ch]=0; break;
  case SEQ_G: h[ch]=85; LL[ch]=127; ss[ch]=255; glow[ch]=0; break;
  case SEQ_B: h[ch]=170; LL[ch]=127; ss[ch]=255; glow[ch]=0; break;
  case SEQ_BLACK: LL[ch]=0; ss[ch]=0; break;
  case SEQ_WHITE: LL[ch]=255; ss[ch]=0; break;
  case SEQ_ALLGLOW: LL[ch]=127; ss[ch]=255; glow[ch]=5; break;
  case SEQ_FASTGLOW: LL[ch]=127; ss[ch]=255; glow[ch]=2; break;
  case SEQ_SETX: if (seq_mode==SEQ_SETX) seq_mode=SEQ_PON; else { seq_mode=SEQ_SETX; seq_xmode = 0; } break;
  }
  //putf("%02X %d:hsl%d,%d,%d\n",(int)b,ch,h[ch],ss[ch],LL[ch]);
}

void seq_move()
{
    L1.hsl(h[0],ss[0],LL[0]);
    L2.hsl(h[1],ss[1],LL[1]);
    L3.hsl(h[2],ss[2],LL[2]);
    L4.hsl(h[3],ss[3],LL[3]);
    if (glow[0] && !(gcnt[0]--)) {gcnt[0]=glow[0]; h[0]++; if (h[0]>256) h[0]-=256;}
    if (glow[1] && !(gcnt[1]--)) {gcnt[1]=glow[1]; h[1]++; if (h[1]>256) h[1]-=256;}
    if (glow[2] && !(gcnt[2]--)) {gcnt[2]=glow[2]; h[2]++; if (h[2]>256) h[2]-=256;}
    if (glow[3] && !(gcnt[3]--)) {gcnt[3]=glow[3]; h[3]++; if (h[3]>256) h[3]-=256;}
}

byte eerd(int a) { return EEPROM.read(a); }
void eewr(int a, byte d) { EEPROM.write(a,d); }

void ee_sav()
{
  int i;
  byte* p;
  dbv = DBVERSION;

  i=2;
  p = (byte*)(void*)&dbv;  eewr(i++, *p++);  eewr(i++, *p++);
  p = (byte*)(void*)h;    for(int j=0; j < 4; j++) {eewr(i++, *p++); eewr(i++, *p++);}
  p = (byte*)(void*)ss;   for(int j=0; j < 4; j++) {eewr(i++, *p++); eewr(i++, *p++);}
  p = (byte*)(void*)LL;   for(int j=0; j < 4; j++) {eewr(i++, *p++); eewr(i++, *p++);}
  p = (byte*)(void*)glow; for(int j=0; j < 4; j++) {eewr(i++, *p++); eewr(i++, *p++);}
  p = (byte*)(void*)ccc;  for(int j=0; j < 4; j++) {eewr(i++, *p++); eewr(i++, *p++);}
  ee_crc();
  eewr(0, lowByte(crc));
  eewr(1, highByte(crc));
}

void ee_load()
{
  int i;
  byte* p;

  ee_crc();
  if (crc != word(eerd(1), eerd(0))) return;
  i=2;
  p = (byte*)(void*)&dbv;  *p++ = eerd(i++);  *p++ = eerd(i++); if (dbv != DBVERSION) return;
  p = (byte*)(void*)h;    for(int j=0; j < 4; j++) { *p++ = eerd(i++);  *p++ = eerd(i++); }
  p = (byte*)(void*)ss;   for(int j=0; j < 4; j++) { *p++ = eerd(i++);  *p++ = eerd(i++); }
  p = (byte*)(void*)LL;   for(int j=0; j < 4; j++) { *p++ = eerd(i++);  *p++ = eerd(i++); }
  p = (byte*)(void*)glow; for(int j=0; j < 4; j++) { *p++ = eerd(i++);  *p++ = eerd(i++); }
  p = (byte*)(void*)ccc;  for(int j=0; j < 4; j++) { *p++ = eerd(i++);  *p++ = eerd(i++); }
}

void ee_crc()
{
  crc = CRC_START;
  for(int i=2, j=0; j < EE_PARN; j++) {
    calculateCRC(eerd(i++));
    calculateCRC(eerd(i++));
  }
}

static const uint16_t crc_table[16] = {
  0x0000, 0xcc01, 0xd801, 0x1400, 0xf001, 0x3c00, 0x2800, 0xe401,
  0xa001, 0x6c00, 0x7800, 0xb401, 0x5000, 0x9c01, 0x8801, 0x4400
};

uint16_t calculateCRC(byte x) 
{ 
  crc ^= x;
  crc = (crc >> 4) ^ crc_table[crc & 0x0f];
  crc = (crc >> 4) ^ crc_table[crc & 0x0f];
  return crc; 
}

