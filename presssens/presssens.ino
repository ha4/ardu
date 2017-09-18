
#include <EEPROM.h>
const int eepa = 5;
uint8_t volatile a=128, b=128;

// crc8-dallas 0x31=x^8+x^5+x^4+1; reversed=0x8C
//uint8_t crc8t[16]={0,0x9D,0x23,0xBE,0x46,0xDB,0x65,0xF8,0x8C,0x11,0xAF,0x32,0xCA,0x57,0xE9,0x74};
// crc8-ccitt  0x07=x^8+x^2+x+1; reversed=0xE0
uint8_t crc8t[16]={0,0x1C,0x38,0x24,0x70,0x6C,0x48,0x54,0xE0,0xFC,0xD8,0xC4,0x90,0x8C,0xA8,0xB4};
uint8_t crc8(uint8_t *p, int n){uint8_t c=0; for(;n--;p++){c^=*p;c=(c>>4)^crc8t[c&15];c=(c>>4)^crc8t[c&15];}return c;}

uint8_t eep_chk()
{
  uint8_t x[3];
  for (int i=0; i<3; i++) x[i]=EEPROM.read(eepa+i);
  if (crc8(x,3)!=0) return 0;
  a = x[0];
  b = x[1];
  return 1;
}

void eep_wr()
{
  uint8_t x[3];
  x[0]=a;
  x[1]=b;
  x[2]=crc8(x,2);
  for (int i=0; i<3; i++) EEPROM.write(eepa+i,x[i]);
}

// freq meter timer0 setup, timer1 setup
uint32_t volatile timerc0;
uint32_t volatile timerc1;
uint16_t volatile timerp1;
void timers_setup()
{
  timerc0 = 0;
  timerc1 = 0;
  timerp1 = 0;
  // timer0 interrupt
   OCR0B = 10; // every 256 clk/precaler interrupt
   TIFR0 |= 1 << OCF0B; // clear flag
   TIMSK0 |= 1<<OCIE0B; // set COMP-B interrupts
   // timer1 clock source T1 (PIN5)
   TCCR1A = 0;
   TCCR1B = 0b111 << CS10; // 0b111 external T1, rising edge 0x
   //TCCR1B = 0b001 << CS10; // 0b001 clk/1
   // clear timer1
   TCNT1H = 0; // latch high part
   TCNT1L = 0;
   // timer2 high frequency
  // TCCR2A=0x01; // wgm2[2:0]=0x01 
  // TCCR2B=0x01; // cs2[2:0]=0x01 (wgm22=0)
   
}

SIGNAL(TIMER0_COMPB_vect)
{
  uint8_t low;
  uint16_t m,d;

  low = TCNT1L; // catch high byte
  m = (TCNT1H << 8) | low;
  d = m - timerp1;
  timerp1=m;
  timerc1 += d;
  timerc0++;
}

uint32_t freq_t1()
{
  uint8_t oldSREG = SREG;
  uint32_t a,b;
  uint64_t f;
  cli();
  if (timerc0 >= 975) {
    a=timerc1;
    b=timerc0;
    timerc1=timerc0=0;
    SREG = oldSREG;
    f = a;
    f *= 16000000;
    f /= b*16384;
    return f;
  } else {
    SREG = oldSREG;
    return 0xFFFFFFFF;
  }
}

void setup()
{
  Serial.begin(115200);
  timers_setup();
  pinMode(5, INPUT);
//  pinMode(8, OUTPUT);
//  tone(8,1000);
  if (!eep_chk()) {
    Serial.println("eeprom wrong");
  }
  analogWrite(3,a);
  analogWrite(11,b);
}

void loop()
{
  unsigned long frequency;
  
  frequency = freq_t1();
  if (frequency!=0xFFFFFFFF) {
    Serial.println(frequency);
  }
  if (Serial.available()) iloop(Serial.read());
}

void iloop(char c)
{
  int x;
  switch(c){
    case 'a': case 'A': x=Serial.parseInt(); a=x; eep_wr(); analogWrite(3,a); break;
    case 'b': case 'B': x=Serial.parseInt(); b=x; eep_wr(); analogWrite(11,b); break;
    case '?': case 'h': Serial.print('A'); Serial.println(a); Serial.print('B'); Serial.println(b); break;
  }
}

void loop1()
{
  unsigned long tl, th, tn, frequency;
  int n;
  
  tn=0;
  for(n=1000; --n > 0;) {
    tl=pulseIn(5, 0, 250000);
    th=pulseIn(5, 1, 250000);
    if (tl==0 || th==0) break;
    tn+=tl+th;
  }
  if (tn != 0) {
    frequency=1000000000/tn;
    Serial.println(frequency);
  }
}


