#include "PinChangeInt.h"

/*
    +5v ---[4.7k]-------------.  
 pulse ----------------------+
 ,--------[15k]----|--------------------.
 `-#1        #4----'                    |
 .-#2 PC814  #3--GND                    |
 `----------[15k]------------------.    |
 |    |
 out -[220]-#1        #6----------T1--------+----|---o L
 gnd -------#2 MOC052 #5          T2------HEATER-+---o N
 #3        #4--[1.5k]--Gate
 */

#define ADC A2
#define DETECT 14  //zero cross detect
//#define DETECT 2  //zero cross detect
#define GATE 15    //TRIAC gate

#define PULSE 4   //trigger pulse width (counts)
void zeroCrossingInterrupt();
static volatile uint16_t acc;
static volatile uint16_t mv;
static volatile uint16_t fan;
static volatile uint16_t temp;
static volatile uint16_t adc;
static float kp,ki,kd;
static volatile bool run;
static volatile bool outpid;
static volatile bool hold;
static volatile bool biperiod;

void init_acpdm()
{
  mv = 0;
  // set up pins
  pinMode(DETECT, INPUT);     //zero cross detect
  digitalWrite(DETECT, 1); //enable pull-up resistor
  digitalWrite(GATE, 0);
  pinMode(GATE, OUTPUT);      //TRIAC gate control

  attachPinChangeInterrupt(DETECT,zeroCrossingInterrupt, CHANGE);  
  //    attachInterrupt(0,zeroCrossingInterrupt, FALLING);  

}  

bool acpdm()
{
  static bool biperiod = 0;

  if (biperiod) {
    biperiod=0; 
    return 1;
  }
  
  uint16_t n = acc+mv; 
  acc=n & 0xFFF;
  if (n&0x1000) {
    biperiod=1;
    return 1;
  } 
  return 0;
}

void zeroCrossingInterrupt()
{
  if(digitalRead(DETECT) && acpdm())
    digitalWrite(GATE, 1); 
  else
    digitalWrite(GATE, 0);
}
 
void serial_process()
{
  switch(Serial.read()) {
  case 'm': mv=Serial.parseInt();  break;
  case 'f': fan=Serial.parseInt(); break;
  case 't': temp=Serial.parseInt();break;
  case 'r': run=Serial.parseInt(); break;
  case 'p': kp=Serial.parseFloat();break;
  case 'i': ki=Serial.parseFloat();break;
  case 'd': kd=Serial.parseFloat();break;
  case 'o': outpid=!outpid; break;
  case 's': run=0; mv=0; break;
  }
}

#define INTMAX 4095
#define MVMAX 4095
void pid_process()
{
  int16_t e,d,o;
  static int16_t pe=0;
  static float i=0;
  adc = analogRead(ADC);
  if (!run) return;
  e = temp - adc;
  i += ki*e; if (i<0)i=0; if( i > INTMAX) i=INTMAX;
  if (ki==0)i=0;
  d = e-pe; pe = e;
  o = kp*e;
  o += i;
  o += kd*d;
  if (o < 0) o=0; if(o > MVMAX) o = MVMAX;
  mv  = o;    
  if (outpid) {
    char buf[80], a1[10], a2[10], a3[10], a4[10];
    sprintf(buf,"pid %s %s %s %d %s %d",dtostrf(kp,-8,3,a1),dtostrf(ki,-8,3,a2),dtostrf(kd,-8,3,a3),e,dtostrf(i,-8,3,a4),d);
    Serial.println(buf);
  }
}

void report()
{
  char buf[32];
  sprintf(buf,"%d %d %d %d %d %d",run,hold,temp,adc,fan,mv);
  Serial.println(buf);
}

void setup()
{
  Serial.begin(115200);
  analogReference(INTERNAL);
  init_acpdm();
  fan = 0; temp = 0; run = 0; adc = 0; hold=0; outpid=0;
  kp=27; ki=4.05; kd=27; // Ku=45, Tu=8
}

uint32_t ms500;
void loop()
{
  uint32_t ms=millis();
  if (Serial.available()) serial_process();
  if (ms-ms500 >= 500) { ms500=ms; pid_process(); report(); }
}


