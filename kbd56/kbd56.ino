
  uint32_t x1,x0;

enum { // A7+D2, A6+D3 connectted
  ROW0=14, ROW1=15, ROW2=16, ROW3=17, 
  ROW4=18, ROW5=19, ROW6=3, ROW7=2, 
  ROWS=8 };

byte kbdmap[ROWS];
byte tmpmap[ROWS];
uint32_t kbdt;

byte rowpin(byte n)
{ 
  switch(n) {
    default: case 0: return ROW0; case 1: return ROW1; case 2: return ROW2; case 3: return ROW3;
  }
}

byte kbd_row(byte i)
{
    switch(i) {
      default: case 0: return ROW0;
      case 1: return ROW1;
      case 2: return ROW2;
      case 3: return ROW3;
      case 4: return ROW4;
      case 5: return ROW5;
      case 6: return ROW6;
      case 7: return ROW7;
    }
}

/* 4bit scan: 104 us 
 * 8bit scan: 260 us
 */
byte kbd_scan_analog()
{
  byte c = 0;
  for(byte i=0; i < ROWS; i++) {
    // scan row N
    byte n=kbd_row(i), r = 0;
    digitalWrite(n, 0); // pin Low 
    pinMode(n, OUTPUT);
    ADMUX = (ADMUX & 0xF0);
    for(byte j=0; j < ROWS; j++) { // readrow loop
      ADMUX = (ADMUX & 0xF0) | (j & 0x07);
      delayMicroseconds(1);
      if (j!=i) { 
        r = (r<<1);
        if (ACSR & _BV(ACO)) r |= 1;
      }
    }
    tmpmap[i]=r;
    pinMode(n, INPUT_PULLUP); 
    
    c |= kbdmap[i]^tmpmap[i]; 
  }
  return c;
}

/* 4bit scan: 140 us 
 * 8bit scan: 470 us
 */
byte kbd_scan()
{
  byte c = 0;
  for(byte i=0; i < ROWS; i++) {
    // scan row N
    byte n=kbd_row(i), r = 0;
    digitalWrite(n, 0); // pin Low 
    pinMode(n, OUTPUT);
    for(byte j=0; j < ROWS; j++) // readrow loop
      if (j!=i)
        r = (r<<1) | (1^digitalRead(kbd_row(j)));
    tmpmap[i]=r;
    pinMode(n, INPUT_PULLUP); 
    
    c |= kbdmap[i]^tmpmap[i]; 
  }
  return c;
}

/*
 * common routines
 */
void printbin(word b, byte n)
{
  word h=1<<(n-1);
  for(int i=0; i < n; i++, h>>=1) {
    Serial.print(' ');
    Serial.print((b&h)!=0);
  }
  Serial.println();
}

void keyboard_process()
{
  byte c;
  
  x0=micros();
//  c = kbd_scan_extender();
//  c = kbd_scan();
//  c = kbd_scan_twi();
  c = kbd_scan_analog();
  if (c) {
  x1=micros();
    Serial.print(" scan:");
    Serial.print(x1-x0);
    Serial.print(" ------ ");
    Serial.println((x1-kbdt)*0.001);
    kbdt=x1;
    for(int i=0; i < ROWS; i++) { kbdmap[i]=tmpmap[i]; printbin(kbdmap[i], 7); }
  }
}

void analog_init()
{
  analogReference(INTERNAL);
  analogRead(0);
  // analog comparator setup
  //ADCSRB |= 0<<ACME; // analog comp multiplexer disable, AIN1(-)=D7 input
  //pinMode(7, INPUT_PULLUP);
 // pinMode(6, INPUT_PULLUP);
  ADCSRB |= _BV(ACME); // analog comp multiplexer enable, AIN1(-)=D7 unused
  ADCSRA &=~_BV(ADEN); // disable ADC to use ADMUX by AC
  ACSR &= ~(_BV(ACD)|_BV(ACBG));  pinMode(6, INPUT_PULLUP); // no bangap, external reference  AIN0(+)=D6
}

void setup()
{
  Serial.begin(115200);

//  analog comparator
  analog_init();

  for(int i=0; i < ROWS; i++) kbdmap[i]=0;
}

void loop()
{
  keyboard_process();
}


