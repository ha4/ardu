
#include "iv8.h"
#include "upid.h"

uint32_t stb=0;
uint32_t sut=0;

uAfilter kU(4);
uAfilter aU(4);
uPID uk;
uPID ua;

void setup()
{
  analogReference(INTERNAL);
 #ifdef USE_SERIAL
  Serial.begin(57600);
 #endif
  uk.set(850);
  uk.begin(0.2, 1.0, 0.0, 1e-3);
  uk.limits(0,220);
  ua.set(20000);
  ua.begin(0.02, 0.11, 0.0, 1e-3);
  ua.limits(0,120);
}

void loop()
{
  uint32_t t;
  t = millis();

#ifdef USE_SERIAL
  if (Serial.available()) i_serial(Serial.read());
#endif
  if (t-stb>=1) stb=t, ustab1(), ustab2();
  if (t-sut>333) sut=t, ushow();
}

void i_serial(char c)
{
#ifdef USE_SERIAL
  int x1;
  switch(c){
  case 'o': x1=Serial.parseInt(); Serial.print(":o"); Serial.println(x1); digitalWrite(U1_PWM,x1); break;
  case 'u': x1=Serial.parseInt(); Serial.print(":u"); Serial.println(x1); digitalWrite(U2_PWM,x1); break;
  case 'e': if (Serial.parseInt()) {Serial.print(":ena"); uk.enable(1); ua.enable(1); } else {Serial.print(":dis"); uk.enable(0); ua.enable(0); } break;
  case '?': Serial.print("TCCR1b:"); Serial.println(TCCR1B,BIN); break;
  }
#endif
}

void ustab1()
{
  kU.filter025(analogRead(U1_PIN));
  int32_t uk_1=kU.Fy*UREF/4096; /// y=Fy/4
  if (uk_1 <0) uk_1=0; if (uk_1 > UREF) uk_1=UREF;
  uk.pid(uk_1);
  if(uk.enabled)
    analogWrite(U1_PWM, (int)uk.MV());
}
  
void ustab2()
{
  aU.filter025(analogRead(U2_PIN));
  int32_t ua_2=aU.Fy*UREF/4096; /// y=Fy/4
  if (ua_2 <0) ua_2=0; if (ua_2 > UREF) ua_2=UREF;
  ua.pid(ua_2*35.5);
  if(ua.enabled)
    analogWrite(U2_PWM, (int)ua.MV());
}
 
void ushow()
{
#ifdef USE_SERIAL
  Serial.print(" mvk:"); Serial.print((int) uk.mv);
  Serial.print(" uk:");  Serial.print((int) uk.pv);
  Serial.print(" mva:"); Serial.print((int) ua.mv);
  Serial.print(" ua:");  Serial.print((int) ua.pv);
  Serial.println();
#endif
}

