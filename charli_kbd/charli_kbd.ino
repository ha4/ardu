
enum { ROW0=14, ROW1=15, ROW2=16, ROW3=17, ROWS=4 };

word kbdmap[ROWS];
word tmpmap[ROWS];
uint32_t kbdt;

/*
 * common routines
 */
byte rowpin(byte n)
{ 
  switch(n) {
    default: case 0: return ROW0; case 1: return ROW1; case 2: return ROW2; case 3: return ROW3;
  }
}

word readrow(byte n)
{
  word r = 0;
  for(byte i=0; i < ROWS; i++)
    if (i!=n) r = (r<<1) | 1^digitalRead(rowpin(i));
  return r;
}

word readvd(byte n)
{
  word r = 0;
  word x = rowpin(n);
  if (x >= 14) x -= 14; // allow for channel or pin numbers
  ADMUX = (ADMUX & 0xF0) | (x & 0x07);
  ADCSRA |= 1<<ADSC;
  while (bit_is_set(ADCSRA, ADSC));
  x = ADCL;
  ADCSRA |= 1<<ADSC;
  while (bit_is_set(ADCSRA, ADSC));
  x = ADCL;
  x = x | (ADCH << 8);
  //x = analogRead(x);
  // ref=1100mv, 745=800mv threshold
  if (x < 745) return 1; else return 0;  
  return x;
}

word readac(byte n)
{
  word r = 0;
  word x = rowpin(n);
  if (x >= 14) x -= 14; // allow for channel or pin numbers
  ADMUX = (ADMUX & 0xF0) | (x & 0x07);
  delayMicroseconds(1);
  if (ACSR & (1<<ACO)) return 1; else return 0;  
}

void pinLow(byte n)
{
    pinMode(n, OUTPUT);  digitalWrite(n, 0); 
}

/*
 * standard scan
 */
void kbd_scan_std()
{
  word c = 0;
  for(int i=0; i < ROWS; i++) {
    byte n = rowpin(i);
    word r = 0;
    pinLow(n);
    for(byte j=0; j < ROWS; j++) // readrow
      if (j!=i) r = (r<<1) | (1^digitalRead(rowpin(j)));
    tmpmap[i]=r;
    pinMode(n, INPUT_PULLUP); 
  }
}

/*
 *  analog scan
 */ 
void kbd_scan_analog()
{
  for(byte i=0; i < ROWS; i++) {
    // scan row N
    byte n = rowpin(i);
    word r = 0;
    pinLow(n);
    for(byte j=0; j < ROWS; j++) // readrow
      if (j!=i) r = (r<<1) | readac(j); // readac(j);
    tmpmap[i]=r;
    pinMode(n, INPUT_PULLUP); 
  }
}

/* 
 * charlieplex scan 
 */
void kbd_scan_chpx()
{
  for(byte i=0; i< ROWS; i++) {
    byte n=rowpin(i);
    word r = 0;
    pinLow(n);
    for(byte j=0; j < ROWS; j++) {
      if (j==i) continue;
      byte m = rowpin(j);
      pinMode(m, INPUT_PULLUP);
      r = (r<<3) | readrow(i);
      pinMode(m, INPUT);
    }
    tmpmap[i] = r;
    pinMode(n, INPUT);
  }
}

void kbd_scan_chpx2()
{
  for(byte i=0; i< ROWS; i++) {
    byte n=rowpin(i);
    word r = 0;
    pinLow(n);
    for(byte j=0; j < ROWS; j++) {
      if (j==i) continue;
      byte m = rowpin(j);
      pinMode(m, INPUT_PULLUP);
      r = (r<<1) | readvd(j);
      pinMode(m, INPUT);
    }
    tmpmap[i] = r;
    pinMode(n, INPUT);
  }
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

word kbd_cmp()
{
  word c = 0;
  for(byte i=0; i < ROWS; i++)
    c |= kbdmap[i]^tmpmap[i]; 
  return c;
}

void keyboard_process()
{
  uint32_t x;
  if (kbd_cmp()) {
    x=micros();
    Serial.print("------");
    Serial.println((x-kbdt)*0.001);
    kbdt=x;
    for(int i=0; i < ROWS; i++) { kbdmap[i]=tmpmap[i]; printbin(kbdmap[i], ROWS); }
  }
}

void ac_setup()
{
  //ADCSRB |= 0<<ACME; // analog comp multiplexer disable, AIN1(-)=D7 input
  //pinMode(7, INPUT_PULLUP);
 // pinMode(6, INPUT_PULLUP);
  ADCSRB |= 1<<ACME; // analog comp multiplexer enable, AIN1(-)=D7 unused
  ADCSRA &=~(1<<ADEN);// disable ADC to use ADMUX by AC
  ACSR |= (0<<ACD)|(0<<ACBG);  pinMode(6, INPUT_PULLUP); // no bangap, external reference  AIN0(+)=D6
  //ACSR |= (0<<ACD)|(1<<ACBG);  // bangap to (+), AIN0(+)=D6 unused
}

void vd_setup()
{
  analogReference(INTERNAL);
  analogRead(0);
}

void setup()
{
  Serial.begin(115200);
//  vd_setup();
//  ac_setup();
  for(int i=0; i < ROWS; i++) tmpmap[i]=kbdmap[i]=0;
}

void loop()
{
//   kbd_scan_std();
//  kbd_scan_chpx2();
//  kbd_scan_analog();
//  kbd_scan_chpx();
  keyboard_process();
}
