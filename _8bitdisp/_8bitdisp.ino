#include "usiTwiSlave.h"

uint8_t disp[4];

const uint8_t addr = 0x6f;
const uint8_t div1khz = 124; // FOSC/64/1000-1

#if defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
// const uint8_t ledpin[] = /* h,a..g */ {7, 8, 9, 10,  3, 2, 1, 0};
volatile uint8_t d = 0;
volatile uint8_t c = 0;

void scanner()
{
  if (d > 8) { d = 0; c=0x80; }
  const uint8_t x = disp[d&3] & ((d&4)?0xF0:0x0F);
  PORTA = (PORTA & 0x70) | (x & 0x8F) ;
  DDRA = (DDRA & 0x70) | ((x | c) & 0x8F);
  PORTB = (PORTB & 0xF8) | ((x>>4) & 7);
  DDRB = (DDRB & 0xF8) | (((x | c)>>4) & 7);
  d++; c>>=1;
}
#else
const uint8_t ledpin[] = /* h,a..g */ {2, 3, 4, 5,   6, 7, 8, 9};

void scanpin(const uint8_t c, const uint8_t p, const uint8_t x, const uint8_t s)
{
      if (c == p) { pinMode(p, OUTPUT); digitalWrite(p, 0); }
      else {
        if (x & s) { pinMode(p, OUTPUT); digitalWrite(p, 1); }
        else pinMode(p, INPUT_PULLUP);
      }
}

void scanner() {
  static uint8_t d = 0;
  if (d >= 8) d = 0;
  const uint8_t x = disp[d&3] & ((d&4)?0xF0:0x0F);
  const uint8_t c = ledpin[d];  
  for(uint8_t i=0, s=0b10000000; s; i++, s>>=1)
      scanpin(c,ledpin[i],x,s);  
  d++;
}
#endif

volatile uint32_t cntr1k=0;

void timer_setup()
{
  noInterrupts(); // Set up timer 1 in mode 2 (CTC mode)
//  GTCCR = 0;
  TCCR0B = 0;				// set the mode, clock stopped for now
  TCNT0 = 0;
  OCR0A = div1khz;
  OCR0B = 0;
  TIFR0 = _BV(OCF0A);			// clear any pending interrupt
  TIMSK0 = _BV(OCIE0A);			// enable the timer 0 compare match A interrupt

//s  TIFR0 = _BV(TOIE0);			// clear any pending interrupt
//  TIMSK0 = _BV(TOV0);			// enable the timer 0 compare match A interrupt
  
  TCCR0A = _BV(WGM01);			// no direct outputs, mode 2
  TCCR0B = _BV(CS01)|_BV(CS00);	// start the clock, prescaler = 64
  interrupts();
}


static int cnt = 0;
const uint8_t digs[] = {
  0b1111110, // 0 abcdefg
  0b0110000, // 1
  0b1101101, // 2
  0b1111001, // 3
  0b0110011, // 4
  0b1011011, // 5
  0b1011111, // 6
  0b1110000, // 7
  0b1111111, // 8
  0b1111011  // 9
};

void demo()
{
    cnt++; if (cnt > 99) cnt = 0;
    const uint8_t d = disp[0]&0x80;
    disp[0]=digs[cnt/10] | (disp[1]&0x80);
    disp[1]=digs[cnt%10] | (disp[2]&0x80);
    disp[2]=digs[(99-cnt)/10] | (disp[3]&0x80);
    disp[3]=digs[(99-cnt)%10] | d;
}


bool every300() {
  static uint16_t t0=0;
  uint16_t t=cntr1k;
  if (t-t0 > 300) { t0=t; return 1; }
  return 0;
}
bool every2() {
  static uint16_t t02=0;
  uint16_t t=cntr1k;
  if (t-t02 > 2) { t02=t; return 1; }
  return 0;
}

void setup()
{
  disp[0]=0x80;
  disp[1]=0xff;
  disp[2]=0;
  disp[3]=0x80;
  usiTwiSlaveInit(addr);
//  timer_setup();
}

void loop()
{
//    if(every2())  
 scanner();
/// delay(1);
//  if (every300()) demo();
//  if (usiTwiDataInReceiveBuffer()) usiTwiReceiveByte();
}

ISR(TIMER0_COMPA_vect)
{
  // scanner();
  cntr1k++;
}

