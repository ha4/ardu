enum {c1=7, c2=8, c3=12, c4=13, aa=3, ab=4, ac=5, ad=6, ae=9, af=10, ag=11, ap=2, t1=14, t2=15, t3=16 };

char buf[5];
byte pt, ptshow, pt_en, i;
unsigned long t_ms,t_s;
int  tmr,tmr0;

void setup()
{
  Serial.begin(9600);
  Serial.setTimeout(10);
  pinMode(c1, OUTPUT);
  pinMode(c2, OUTPUT);
  pinMode(c3, OUTPUT);
  pinMode(c4, OUTPUT);
  pinMode(aa, OUTPUT);
  pinMode(ab, OUTPUT);
  pinMode(ac, OUTPUT);
  pinMode(ad, OUTPUT);
  pinMode(ae, OUTPUT);
  pinMode(af, OUTPUT);
  pinMode(ag, OUTPUT);
  pinMode(ap, OUTPUT);
  pinMode(t1, INPUT_PULLUP);
  pinMode(t2, INPUT_PULLUP);
  pinMode(t3, INPUT_PULLUP);
  pt=0; i=0;
  
  ptshow=0; pt_en=0;
  msg("----");
  tmr0=5;
  tmr=-3;
}

void msg(char *s)
{
  sprintf(buf,s);
  ptshow = 0;
}

void loop1s()
{
  if (tmr0 !=0) { tmr0--; return; }
  if (tmr == -3) return;
  if (tmr == -1) msg(" End");
  if (tmr == -2) msg("----");
  if (tmr < 0) { pt_en=0; tmr=-3; }
  else {
    sprintf(buf, "%02d%02d", tmr/60, tmr%60);
    ptshow=1; pt_en=1;
    tmr--;
  }
}

void loop2ms()
{
  pt=0; digitSP();
  digitalWrite(c1,(i==0)?0:1);
  digitalWrite(c2,(i==1)?0:1);
  digitalWrite(c3,(i==2)?0:1);
  digitalWrite(c4,(i==3)?0:1);
  if (i>=2) pt=ptshow;
  switch(buf[i]) {
    case ' ': default: digitSP(); break;
    case '-': digitML(); break;
    case '0': digit0(); break;
    case '1': digit1(); break;
    case '2': digit2(); break;
    case '3': digit3(); break;
    case '4': digit4(); break;
    case '5': digit5(); break;
    case '6': digit6(); break;
    case '7': digit7(); break;
    case '8': digit8(); break;
    case '9': digit9(); break;
    case 'E': digitE(); break;
    case 'n': digitN(); break;
    case 'd': digitD(); break;
  }
  if(++i > 3) { i=0; loop10ms(); }
}

void loop10ms()
{
  if (digitalRead(t1)==0) tmr = 5;
  if (digitalRead(t2)==0) tmr = 15;
  if (digitalRead(t3)==0) tmr = 90;
}

void loop()
{
  unsigned long t;
  t=micros();
  if (t-t_ms >= 2000) { t_ms+=2000; loop2ms(); }
  t=millis();
  if (t-t_s  >= 1000) { t_s+=1000; loop1s(); }
  if(Serial.available()) tmr = Serial.parseInt();
}

void digitSP()
{
  digitalWrite(aa,0);
  digitalWrite(ab,0);
  digitalWrite(ac,0);
  digitalWrite(ad,0);
  digitalWrite(ae,0);
  digitalWrite(af,0);
  digitalWrite(ag,0);
  digitalWrite(ap,pt);
}

void digit0()
{
  digitalWrite(aa,1);
  digitalWrite(ab,1);
  digitalWrite(ac,1);
  digitalWrite(ad,1);
  digitalWrite(ae,1);
  digitalWrite(af,1);
  digitalWrite(ag,0);
  digitalWrite(ap,pt);
}
void digit1()
{
  digitalWrite(aa,0);
  digitalWrite(ab,1);
  digitalWrite(ac,1);
  digitalWrite(ad,0);
  digitalWrite(ae,0);
  digitalWrite(af,0);
  digitalWrite(ag,0);
  digitalWrite(ap,pt);
}
void digit2()
{
  digitalWrite(aa,1);
  digitalWrite(ab,1);
  digitalWrite(ac,0);
  digitalWrite(ad,1);
  digitalWrite(ae,1);
  digitalWrite(af,0);
  digitalWrite(ag,1);
  digitalWrite(ap,pt);
}
void digit3()
{
  digitalWrite(aa,1);
  digitalWrite(ab,1);
  digitalWrite(ac,1);
  digitalWrite(ad,1);
  digitalWrite(ae,0);
  digitalWrite(af,0);
  digitalWrite(ag,1);
  digitalWrite(ap,pt);
}
void digit4()
{
  digitalWrite(aa,0);
  digitalWrite(ab,1);
  digitalWrite(ac,1);
  digitalWrite(ad,0);
  digitalWrite(ae,0);
  digitalWrite(af,1);
  digitalWrite(ag,1);
  digitalWrite(ap,pt);
}
void digit5()
{
  digitalWrite(aa,1);
  digitalWrite(ab,0);
  digitalWrite(ac,1);
  digitalWrite(ad,1);
  digitalWrite(ae,0);
  digitalWrite(af,1);
  digitalWrite(ag,1);
  digitalWrite(ap,pt);
}
void digit6()
{
  digitalWrite(aa,1);
  digitalWrite(ab,0);
  digitalWrite(ac,1);
  digitalWrite(ad,1);
  digitalWrite(ae,1);
  digitalWrite(af,1);
  digitalWrite(ag,1);
  digitalWrite(ap,pt);
}
void digit7()
{
  digitalWrite(aa,1);
  digitalWrite(ab,1);
  digitalWrite(ac,1);
  digitalWrite(ad,0);
  digitalWrite(ae,0);
  digitalWrite(af,0);
  digitalWrite(ag,0);
  digitalWrite(ap,pt);
}
void digit8()
{
  digitalWrite(aa,1);
  digitalWrite(ab,1);
  digitalWrite(ac,1);
  digitalWrite(ad,1);
  digitalWrite(ae,1);
  digitalWrite(af,1);
  digitalWrite(ag,1);
  digitalWrite(ap,pt);
}
void digit9()
{
  digitalWrite(aa,1);
  digitalWrite(ab,1);
  digitalWrite(ac,1);
  digitalWrite(ad,1);
  digitalWrite(ae,0);
  digitalWrite(af,1);
  digitalWrite(ag,1);
  digitalWrite(ap,pt);
}
void digitE()
{
  digitalWrite(aa,1);
  digitalWrite(ab,0);
  digitalWrite(ac,0);
  digitalWrite(ad,1);
  digitalWrite(ae,1);
  digitalWrite(af,1);
  digitalWrite(ag,1);
  digitalWrite(ap,pt);
}
void digitN()
{
  digitalWrite(aa,0);
  digitalWrite(ab,0);
  digitalWrite(ac,1);
  digitalWrite(ad,0);
  digitalWrite(ae,1);
  digitalWrite(af,0);
  digitalWrite(ag,1);
  digitalWrite(ap,pt);
}
void digitD()
{
  digitalWrite(aa,0);
  digitalWrite(ab,1);
  digitalWrite(ac,1);
  digitalWrite(ad,1);
  digitalWrite(ae,1);
  digitalWrite(af,0);
  digitalWrite(ag,1);
  digitalWrite(ap,pt);
}
void digitML()
{
  digitalWrite(aa,0);
  digitalWrite(ab,0);
  digitalWrite(ac,0);
  digitalWrite(ad,0);
  digitalWrite(ae,0);
  digitalWrite(af,0);
  digitalWrite(ag,1);
  digitalWrite(ap,pt);
}


