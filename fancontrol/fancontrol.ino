
#define UREF 1096
#define OPWM_PIN 9
#define UCC_PIN A1
#define UHI_PIN A0
#define UICC_PIN A2

class uAfilter {
public:
  int32_t Fy, y;
  int32_t ra;
  uAfilter(int32_t reverse_a) { ra = reverse_a; Fy=0; y=0; }
  int16_t filter(int16_t x);
  int16_t filter025(int16_t x);
  int16_t filter0125(int16_t x);
};


void timer1_cfg(uint8_t mode, uint8_t clks)
{
  TCCR1A = mode & 3;
  TCCR1B = ((mode & 12)<<1) | clks; //
}

uAfilter Fucc(4), Fuhi(4), Ficc(4);
uint16_t ucc, uhi, ufan, icc, umv;

void setup()
{
    timer1_cfg(2,1);   // mode: 5, FastPWM8 62.5 kHz; mode 1, Phase Corr.PWM8, 31.25kHz
    analogReference(INTERNAL);
    analogWrite(9, 0); //COM1A:10 OC1A:PB1,#13,D9 OC1B:PB2,#14,D10
    Serial.begin(57600);
}

void i_serial(char c)
{
  int x1;

  switch(c){
    case 'o':    x1 = Serial.parseInt();  Serial.print(":ocr");    Serial.println(umv=x1); break;
  }
}

void process()
{
  uint32_t x1;
  Fucc.filter025(analogRead(UCC_PIN));
  x1 = Fucc.Fy*UREF/4096;
  ucc = x1*4866/100; // divider 470k/10k
  Fuhi.filter025(analogRead(UHI_PIN));
  x1 = Fuhi.Fy*UREF/4096;
  uhi = x1*4824/100; // divider 470k/10k
  Ficc.filter025(analogRead(UICC_PIN));
  x1 = Ficc.Fy*UREF/4096;
  icc = x1*100/47; // resistor 0.47ohm
  if (uhi>ucc) ufan = uhi-ucc; else ufan = 0;
  analogWrite(OPWM_PIN, umv); 
}

void report()
{
  Serial.print("ucc:"); Serial.print(ucc);
  Serial.print(" uhi:"); Serial.print(uhi);
  Serial.print(" ufan:"); Serial.print(ufan);
  Serial.print(" icc:"); Serial.print(icc);
  Serial.print(" MV:"); Serial.print(umv);
  Serial.println();
}

uint32_t tsta=0;
uint32_t tstb=0;
void loop()
{
  uint32_t t;
  t = millis();
  if (Serial.available()) i_serial(Serial.read());
  if (t-tstb >=1)   tstb=t, process();
  if (t-tsta >=500) tsta=t, report();
}


int16_t uAfilter::filter(int16_t x)
{
  Fy+=x; Fy-=y;
  y=Fy/ra;
  return y;
}

int16_t uAfilter::filter025(int16_t x)
{
  Fy+=x; Fy-=y;
  y=Fy/4;
  return y;
}

int16_t uAfilter::filter0125(int16_t x)
{
  Fy+=x; Fy-=y;
  y=Fy/8;
  return y;
}

