/*
 paraphase signal
 */

/* TIMER0 fast-pwm,
   OC0B PD5 - inverse 0b11 set-on-COM0B-clear-on-top
   OC0A PD6 - direct 0b10 (clear-on-COM0A-set-on-top,
   
   PD5 -R10K--||--*----+---R24K---+--------- A0
   PD6 -R10K------~   ===         ~->|---|
                      _|_          
                      
   f PWM = 980Hz
   Tau = 10ms
   tset = 300ms
   Vt=kT/q
   I=Io(exp(V/nVt)-1
   I=Io*(exp(qV/nkT)-1)
  Ih=Io(exp(qVh/nkT)-1); IL=Io(exp(qVL/nkT)-1)
  Ih/Il=exp(qVh/nkT)-1 / exp(qVL/nkT)-1
  Ih/Io+1 = exp(qVh/nkT); IL/Io+1=exp(qVL/nkT)
  Ih/Io+1 / IL/Io+1 = exp(q(Vh-Vl)/nkT)
  ln(Ih/IL)=qDV/nkT
   DV=nkTln(Ih/IL)/q
   n=1; k/q=1/11604.5
   Dv*11604.5=T*ln(252/84)
   Dv*10562.78=T
   Vbe * kb / (qT) = lnI/Is
   Vbe=q/kb*T*ln(I)
   DeltaVbe = q/kb*T*ln(Ratio(i))
   DVbe=11604.5*T*
   
 */
 
#define UREF 1113

void
setParaphase(uint8_t w)
{
  TCCR0A = (TCCR0A & 0xF) | _BV(COM0A1) | _BV(COM0B1) | _BV(COM0B0);
  OCR0A=OCR0B=w;
}


void
setup()
{
  Serial.begin(115200);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  analogReference(INTERNAL);
  analogRead(A0);
}

uint16_t
measureu()
{
  uint32_t u;
  u=0;

  for(uint16_t n=UREF; n--;)
      u+=analogRead(A0);
  u>>=10;
  return u;
}

void
measuret()
{
  uint16_t  uL, uH, Du;

  setParaphase(83); // 84*4us = 336us of 1024us
  delay(300);
  uL=measureu();

  setParaphase(251); // 252*4us = 1008us of 1024us
  delay(300);
  uH=measureu();

  Du=uH-uL;
  Serial.print(uH); Serial.print(' ');
  Serial.print(uL); Serial.print(' ');
  Serial.print(Du); Serial.print('\n');
}

#define BSZ  30
uint16_t bump[BSZ];

void
loop()
{

  if(Serial.available()) {
    setParaphase(Serial.parseInt());
    for(int8_t a=0; a<BSZ; a++) {
      bump[a]=analogRead(A0);
      delayMicroseconds(10000);
    }
    
    while(Serial.available()) Serial.read();
    Serial.print(':');
    Serial.println((int)OCR0A, DEC);
    for(int8_t a=0; a<BSZ; a++)
      Serial.println(bump[a]);
    Serial.println(':');
  } else 
    measuret();
}

