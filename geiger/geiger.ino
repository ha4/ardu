
#include "eeparam.h"
#include <LCDnum.h>

uint16_t  volatile setp = 193;
byte volatile ir;
unsigned long volatile it1, it2;
float volatile mtime=NAN;
/*
uint32_t volatile cpm=0;
unsigned long volatile tcpm;
*/

const char  parc[] = { 'u'};
void* parv[] =       { (void*)&setp };

enum {
       pin_pwm = 9, pin_u = A6,  /* voltage controller */
       pin_led=-1, pin_trig=3, /* int#1 */
       pin_base = 10, pin_c=13, pin_d=11, pin_i=12,
};

LCDnum panel(pin_base, pin_base);

void setup()
{
  Serial.begin(57600);
  analogReference(INTERNAL);
//  panel.rightToLeft();
//3  #1  PD3:OC2B
//5  #9  PD5:OC0B
//6  #10 PD6:OC0A
// basic:
//9  #13 PB1:OC1A
//10 #14 PB2:OC1B
//11 #15 PB3:OC2A


  TCCR1B = (TCCR1B & 0b11111000) | 0b001; // set PWM frequency @ 31250 Hz for Pins 9 and 10, MODE ?
  //TCCR2B = (TCCR2B & 0b11111000) | 0b001; // set PWM frequency @ 31250 Hz for Pins 11 and 3 (3 not used)

// PIN9 PB1 TIMER1A // mode 1000: phase correct, ICR1
//  analogWrite(9, 128);
  TCCR1B = (TCCR1B & 0b11000000) | _BV(WGM13) | 0b001; // mode 1000, CS1-001(clk)
  TCCR1A = _BV(COM1A1) | (0b00);  // mode 1000
  pinMode(pin_pwm,OUTPUT);
  ICR1 = 1023 ; // 10   bit resolution
  OCR1A = 1; 
  
  lpar();

  ir = 0;
  pinMode(pin_led, OUTPUT);
  pinMode(pin_trig, INPUT); // pinD3, int1
  attachInterrupt(1, gm_tick, FALLING);

}

void gm_tick()
{
    ir = 1;
    it2 = it1;
    it1 = micros();
}

float filter(float y, float x, float a)
{
  if (isnan(y)) return x;
  else return (1-a)*y+a*x;
}

float kp=5, ki=10; // kp=Gain, ki=Gain/Tau
float eprev=0;
uint32_t ty;
float volatile u = 0, mv = 224;

// Dout = kp*Derr + ki*err*dt
float pi_ctrl(float sp, float pv)
{
  float e,de,dt;
  uint32_t tx,t;

  t = micros();

  tx = t-ty;
  ty = t;
  dt = 1e-6*tx;

  e = sp - pv;
  de=e-eprev;
  eprev=e;

  return kp*de+ki*e*dt;
}

void ustab()
{
  int x1;
  x1 = analogRead(pin_u);
  u = filter(u, 1.12*x1, 0.3); // 2*5.1M+10k divider, 1.1v/1024
  mv += pi_ctrl(setp, u);
  if (mv<0)   mv=0;
  if (mv>500) mv=500;
  OCR1A=(int)mv;
//  if ((millis()&511)==0) { Serial.println(u*2.1); }
}

void i_serial(char c)
{
  int x1;
  float x2;

  switch(c){
    case 'o':    x1 = Serial.parseInt();  Serial.print(":ocr");    Serial.println(x1);  mv=x1; break;
    case 'i':    x1 = Serial.parseInt();  Serial.print(":icr");    Serial.println(x1);  ICR1 =x1; break;
    case 'p':    x2 = Serial.parseFloat();  Serial.print(":kp");    Serial.println(x2);  kp=x2; break;
    case 't':    x2 = Serial.parseFloat();  Serial.print(":ki");    Serial.println(x2);  ki=x2; break;
    case 's':    x2 = Serial.parseFloat();  Serial.print(":sp");    Serial.println(x2);  setp=x2; break;
    case 'Z':  clrpar(); break;
    case 'W':  wpar();  break;
    case '?':   
     Serial.print(":ocr");    Serial.println(OCR1A);
     Serial.print(":icr");    Serial.println(ICR1);
     Serial.print(":kp");    Serial.println(kp);
     Serial.print(":ki");    Serial.println(ki);
     Serial.print(":sp");    Serial.println(setp);
     Serial.print(":u");    Serial.println(u*2.1);
     break;
  }
}

/*
#define PULSES_SZ 20
int n_pulses=0, i_pulses=0;
uint32_t pulses[PULSES_SZ];
uint32_t pulsesum=0;

float pulse_new(uint32_t dt)
{
  if (n_pulses < PULSES_SZ) {
    pulses[n_pulses] = dt;
    n_pulses++;
  } else {
    pulsesum -= pulses[i_pulses];
    pulses[i_pulses] = dt;
    i_pulses++;
    if (i_pulses >= PULSES_SZ) i_pulses -= PULSES_SZ;
  }
  pulsesum += dt;
  cpm++;
  
  return 60e6/(pulsesum/n_pulses);
}
*/

void gm_counter()
{
    float cpm,urph,usvph;

    digitalWrite(pin_led, 1);
    it2 = it1 - it2;
    mtime = filter(mtime, it2, 0.1);
    cpm = 60e6/mtime; // counts per minute
    urph = cpm*60.0*0.28/30.0; // 0.28uR/s=30Hz CTC-5
    usvph= urph/102.0; // 102 R = 1Sv (gamma-, beta-)
    
    // type 3  x3 = pulse_new(it2);
    panel.println(usvph);
    Serial.print(usvph);
    Serial.println("uSv/h");
    digitalWrite(pin_led, 0);
}

/*
void gm_cpm()
{
  uint32_t t;
  t = millis();
  if(t-tcpm < 60000) return;
  tcpm=t;
  Serial.print("cpm:");
  Serial.println(cpm);
  cpm = 0;
}
*/

unsigned long volatile tlcd=0;
int lcd_20ms()
{
  uint32_t t;
  t = millis();
  if(t-tlcd < 30) return 0;
  tlcd=t;
  return 1;
}

void loop()
{
  if (Serial.available()) i_serial(Serial.read());
  if (lcd_20ms()) panel.refresh(0);
  ustab();
  if (ir) ir=0, gm_counter();
//  gm_cpm();
}
