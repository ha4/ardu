#include "PinChangeInt.h"

enum { stb = 9, zcd = 8 };

volatile int  cross,sy,e;
volatile int  delta1, delta2;
volatile byte syncx;
volatile uint16_t  prev, prev90, sync, perio, nextz, angle, prevg, delta, phs;
volatile uint32_t  ft;
int32_t  fp,fe,fd;
volatile uint16_t t2;
volatile int16_t t1;

int16_t dfiltr(int32_t *y, int16_t x)
{
  (*y)+= (((int32_t)x*16) - *y) /16;
//  y' = y(1-a)+xa = y - ay + ax = y + ax-ay => y+= ax-ay, a'=1/a
//  ie:  a'y+=x-y/a'
  return (*y) / 16;
}

int16_t udiff(uint16_t a, uint16_t b)
{
    uint16_t q;
    q = a-b;
    if (q <= 32767) return q;
    return q;
}

void zerocross()
{ 
  uint16_t c = TCNT1;
  uint16_t d,c90;
  d = c-prev;
  c90 = c-(d>>1); // c/2+prev/2
  delta = c90-prev90;
  delta = dfiltr(&fd,delta);
  cross++;
  prev = c;
  prev90=c90;
// next90
  c = c90;
  c +=delta;
  OCR1A = c;
}

/* syncro generator */
ISR(TIMER1_COMPA_vect)
{
  uint16_t c = TCNT1;
  OCR1B = c+(delta>>1); // zer0
  e=1;
}

/* fire angle */
ISR(TIMER1_COMPB_vect)
{
  uint16_t c = TCNT1;
  if (e) {
    digitalWrite(stb, LOW);
    c+=200; // 100us pulse
    OCR1B = c;
    e=0;
  } else {
    digitalWrite(stb, HIGH);
  }
}

void setup()
{
//  Serial.begin(115200);
  pinMode(stb, OUTPUT);
  digitalWrite(stb, HIGH);
  pinMode(zcd, INPUT);
  TCCR1A = 0;
  TCCR1B = 2 << CS10; // clk/8
  TCCR1C = 0;
  TCNT1  = 0;
  OCR1A=20000;
  OCR1B=20001;
//  TIMSK1 &= ~((1<<OCIE1A)|(OCIE1B)); // disable OC
  TIMSK1 |=  (1<<OCIE1A)|(1<<OCIE1B); // enable OC
  cross=0;
  ft=0;
  perio=20000;
  fe=0;
  fp=perio*16L;
  fd=perio*16L;
  attachPinChangeInterrupt(zcd, zerocross, CHANGE);
}

char buf[50];

void loop()
{
//  if(cross) { cross=0; sprintf(buf,"%5u\n",delta); Serial.print(buf); } 
//  if (sy) { sy=0; sprintf(buf,"sync\n");  Serial.print(buf); }
}

