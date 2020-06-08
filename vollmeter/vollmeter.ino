#include "U8glib.h"

#define PIN_B1   2
#define PIN_B2   3
#define PIN_B3   4
#define PIN_B4   5
#define PIN_GA   6
#define PIN_GBU  11
#define PIN_GBL  12
#define PIN_ULIM 7
#define PIN_ILIM 8
#define PIN_OUTP 9
#define PIN_OUTN 10
#define DPIN_UI   14
#define APIN_UI   0
#define U_REF 1128 /*mV*/
#define U_VDD 5000 /*mV*/
#define R_UP 4703 /*Ohm*/
#define R_REF 4703 /*Ohm*/
#define IHLIMIT  1020
#define ILLIMIT  100
#define IAMP     11.737
#define UAMPQ     100 /* amp=100/41 */
#define UAMPD     41
#define PWMMAX   0xFFF  /*12bit*/
#define DEBOUNCE 2
#define FL_IAMP  0x0001
#define FL_UAMP  0x0002
#define FL_CNTL  0x0004
#define MODE_OFF   0
#define MODE_POS   1
#define MODE_NEG   6
#define MODE_SHORT 7

//U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_DEV_0|U8G_I2C_OPT_NO_ACK|U8G_I2C_OPT_FAST);	// Fast I2C / TWI
U8GLIB_SSD1306_128X32 u8g(U8G_I2C_OPT_NONE);	// I2C / TWI

static uint16_t cntr;
static uint16_t nval; /* code */
static float ival;   /* uA */
static uint16_t uval; /* code */
static uint16_t uout; /* mv */
static uint16_t uvdd; /* mv */
static uint16_t nvdd; /* code */
static uint8_t  gmod; /* code */
static uint16_t flags;
static uint16_t buttons;
static uint16_t usweep;
static uint16_t dbuttons;
static uint8_t debouncing;
static uint8_t  reference; /* binary */

void setflag(int fl, int val);
void draw(void);
void timer1init();
void timer1set(uint16_t x);
void iSerial(); // serial input
void initADC(uint8_t ch);
int  readADC(); // code return
void acquire(); // measure cycle
void calibrateVDD();
void set_out(uint16_t mv); // output to timer
void input(); // button input
void baction(uint8_t bid, uint8_t bpress); // button action
void control(); // process control
void setgate(uint8_t gmode);


void timer1init()
{
  TCCR1A = 0;
  TCCR1B = 0;
  // OC1A:PB1=D9 fastpwm OC1B:PB2=D10 inverse fastpwm
  TCCR1B = (0b11 << WGM12) | (0b001 << CS10); // mode14, clock source clk/1  : 0b00011001 0x19
  TCCR1A = (0b10 << COM1A0) | (0b11 << COM1B0) | (0b10 << WGM10); // fastPWM mode 14 ICR1=TOP : 0b10110010 0xb2
  ICR1 = PWMMAX;
}

void timer1set(uint16_t x)
{
  register uint8_t xxh, xxl;
  uval = x;
  xxh = highByte(x);
  xxl = lowByte(x);
  OCR1AH = xxh;
  OCR1AL = xxl;
  OCR1BH = xxh;
  OCR1BL = xxl;
}

void setflag(int fl, int val)
{
  uint8_t cbit, port, pin;
  volatile uint8_t *out, *reg;

  if (fl & FL_IAMP) pin = PIN_ILIM;
  else if (fl & FL_UAMP) pin = PIN_ULIM;
  else {
    if (val) flags |= fl; else flags &= ~fl;
    return;
  }
  cbit = digitalPinToBitMask(pin);
  port = digitalPinToPort(pin);
  out = portOutputRegister(port);
  reg = portModeRegister(port);
  if (val) {
    *reg &= ~cbit; // 1 float pin
    *out &= ~cbit;
    flags |= fl;
  } else {
    *reg |= cbit;  // 0 output
    *out &= ~cbit;
    flags &= ~fl;
  }
}

void setgate(uint8_t gmode)
{
  if (gmode & 1) digitalWrite(PIN_GA, 0); else digitalWrite(PIN_GA, 1);
  if (gmode & 2) digitalWrite(PIN_GBL, 0); else digitalWrite(PIN_GBL, 1);
  if (gmode & 4) digitalWrite(PIN_GBU, 0); else digitalWrite(PIN_GBU, 1);
  gmod = gmode&7;
}

void iSerial()
{
  uint16_t x;
  switch (Serial.read()) {
    case 'n':
      x = Serial.parseInt();
      timer1set(x);
      Serial.print("set timer:");
      Serial.println(x);
      break;
    case 'u':
      x = Serial.parseInt();
      set_out(x);
      Serial.print("u output:");
      Serial.println(x);
      break;
    case 'l':
      x = Serial.parseInt();
      setflag(FL_IAMP, x);
      Serial.print("set i-amp:");
      Serial.println(x);
      break;
    case 'o':
      x = Serial.parseInt();
      setflag(FL_UAMP, x);
      Serial.print("set u-amp:");
      Serial.println(x);
      break;
    case 'c':
      calibrateVDD();
      Serial.print("nVDD:");
      Serial.println(nvdd);
      Serial.print(" uVDD:");
      Serial.println(uvdd);
      break;
    case 'r':
      usweep = Serial.parseInt();
      setflag(FL_CNTL, 1);
      calibrateVDD();
      Serial.print("sweep uout to:");
      Serial.println(usweep);
      break;
    case 'm':
      x = Serial.parseInt();
      Serial.print("set mode:");
      Serial.println(x);
      setgate(x);
      break;
    case '?':
      Serial.print("flags:");
      Serial.println(flags);
      Serial.print("timer:");
      Serial.println(uval);
      Serial.print("mode:");
      Serial.println(gmod);
      break;
  }
}

#define SAMPLES_ADC 10
#define SAMPLES_BITDIV 3  /* /8=10-2 min,max samples */
static int resultADC[SAMPLES_ADC];
static uint8_t counterADC;

void initADC(uint8_t ch)
{
  counterADC = 0;
  reference=0b11; // internal 1.1v
   // ADC enable, PRESCALER=111(128) Fadc=125kHz, autotrigger, start, interrupts
  ADCSRA = (1<<ADEN)|(1 << ADSC)|(1<<ADATE)|(1<<ADIF)|(1<<ADIE)|(0b111 << ADPS0);
  ADCSRB = (0<<ACME)|(0b000<<ADTS0); // AC mux disable, Trigger source=free run
  DIDR0 = (1<<APIN_UI);

  ADMUX = (reference << REFS0) | (ch & 0x07);
  //delay(1);
}

ISR (ADC_vect)
{
    resultADC[counterADC++] = ADC;
    if(counterADC >= SAMPLES_ADC) counterADC = 0;
} 

int readADC()
{
  uint16_t adsum, admax, admin;
//  uint8_t low, high;
 
//  ADCSRA |= 1 << ADSC;// start the conversion
//  while (ADCSRA & (1 << ADSC));// ADSC is cleared when the conversion finishes
  // ADIF is set when the conversion finishes
  /*
  while (!(ADCSRA & (1 << ADIF)));
  ADCSRA |= 1<<ADIF;
  low  = ADCL;
  high = ADCH;
  return (high << 8) | low;
  */
//  return resultADC;
  noInterrupts();
  adsum = 0;
  admin=admax=resultADC[0];
  for(int i=0; i < SAMPLES_ADC; i++) {
    uint16_t s=resultADC[i];
    adsum+=s;
    if (admax < s) admax=s;
    if (admin > s) admin=s;
  }
  interrupts();
  adsum -= admax+admin;
  adsum >>= SAMPLES_BITDIV;
  return (uint16_t)adsum;
}

void acquire()
{
  char *p;
  nval = readADC();
  ival = nval;
  ival *= U_REF;
  ival *= 1000;
  ival /= 1024;
  if ((flags&FL_IAMP)==0) ival /= IAMP;
  ival /= R_REF;

  if (nval < 80 && (flags&FL_IAMP)!=0) setflag(FL_IAMP, 0);
  if (nval > 1020 && (flags&FL_IAMP)==0) setflag(FL_IAMP, 1);

  Serial.print(uout);Serial.print(' ');
  Serial.print(nval);Serial.print('=');
  Serial.print(ival);Serial.println("uA");
}

void calibrateVDD()
{
  uint8_t stor = ADMUX;
  uint32_t cv;
  ADMUX = (0b01 << REFS0) | (0b1110);
  delay(3);
  nvdd=readADC();
  ADMUX = stor;
  delay(1);
  cv = nvdd;
  cv = 1024L*U_REF/cv;
  uvdd=cv;
}

void set_out(uint16_t mv)
{
  uint32_t z;

  uout=mv;
  calibrateVDD();
  
  z=mv;
  z*=PWMMAX;
  z*=UAMPD;
  z/=UAMPQ;
  z/=uvdd;

  timer1set(z);
}

void draw(void) {
  // graphic commands to redraw the complete screen should be placed here
  u8g.setFont(u8g_font_unifontr);
  u8g.setPrintPos( 0, 16);
  u8g.print(cntr);
  u8g.setPrintPos( 0, 30);
  u8g.print(ival);u8g.print("uA");
  u8g.setPrintPos( 75, 30);
  u8g.print((float)uout*0.001,2); u8g.print("V");
}

void baction(uint8_t bid, uint8_t bpress)
{
  if (bid==1 && bpress) {
    uout+=500; if (uout>15000)uout=0; set_out(uout);
  }
}

void input()
{
  uint16_t readb=0, chng;
  if (digitalRead(PIN_B1) == 0)readb|=1;
  if (digitalRead(PIN_B2) == 0)readb|=2;
  if (digitalRead(PIN_B3) == 0)readb|=4;
  if (digitalRead(PIN_B4) == 0)readb|=8;
  if(dbuttons != readb) { debouncing=DEBOUNCE; dbuttons = readb; }
  if (debouncing) {
      if (--debouncing) return;
      chng=buttons^dbuttons;
      buttons=dbuttons;
      for(uint16_t b=1; b != 0x10; b<<=1) if (chng&b) baction(b,buttons&b);
  }
}

void control()
{
   if (!(flags & FL_CNTL)) return;
   if (uout < usweep) {
    set_out(uout+10);
    if (uout >= usweep) setflag(FL_CNTL, 0);
   } else if (uout > usweep) {
    set_out(uout-10);
    if (uout <= usweep) setflag(FL_CNTL, 0);
   } else
    setflag(FL_CNTL, 0);
   
}

void setup(void)
{
  initADC(APIN_UI);
  uvdd=U_VDD;
  Serial.begin(9600);
  calibrateVDD();
  // flip screen, if required
  // u8g.setRot180();
  u8g.setColorIndex(1);         // pixel on
  timer1init();
  pinMode(PIN_OUTP, OUTPUT);
  pinMode(PIN_OUTN, OUTPUT);
  pinMode(DPIN_UI, INPUT);
  pinMode(PIN_B1, INPUT_PULLUP);
  pinMode(PIN_B2, INPUT_PULLUP);
  pinMode(PIN_B3, INPUT_PULLUP);
  pinMode(PIN_B4, INPUT_PULLUP);
  digitalWrite(PIN_GA,1);
  digitalWrite(PIN_GBU,1);
  digitalWrite(PIN_GBL,1);
  pinMode(PIN_GA, OUTPUT);
  pinMode(PIN_GBL, OUTPUT);
  pinMode(PIN_GBU, OUTPUT);
  setflag(FL_IAMP, 1);  // low amp
  setflag(FL_UAMP, 1);  // low amp
  setflag(FL_CNTL, 0);  // no process
  buttons = 0;
  dbuttons = 0;
  debouncing=DEBOUNCE;
}

void loop(void)
{
  // picture loop
  u8g.firstPage(); do draw(); while ( u8g.nextPage() );
  if ((cntr & 15) == 0) acquire();
  control();
  if (Serial.available()) iSerial();
  input();
  delay(31);
  cntr++;
}
