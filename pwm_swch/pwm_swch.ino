
void setup()
{
  Serial.begin(57600);
  analogReference(INTERNAL);
//3  #1  PD3:OC2B
//5  #9  PD5:OC0B
//6  #10 PD6:OC0A
// basic:
//9  #13 PB1:OC1A
//10 #14 PB2:OC1B
//11 #15 PB3:OC2A

  Serial.println(TCCR1A,HEX);
  Serial.println(TCCR1B,HEX);

  TCCR1B = (TCCR1B & 0b11111000) | 0b001; // set PWM frequency @ 31250 Hz for Pins 9 and 10, MODE ?
  //TCCR2B = (TCCR2B & 0b11111000) | 0b001; // set PWM frequency @ 31250 Hz for Pins 11 and 3 (3 not used)

// PIN9 PB1 TIMER1A // mode 1000: phase correct, ICR1
//  analogWrite(9, 128);
  TCCR1B = (TCCR1B & 0b11000000) | _BV(WGM13) | 0b001; // mode 1000, CS1-001(clk)
  TCCR1A = _BV(COM1A1) | (0b00);  // mode 1000
  pinMode(9,OUTPUT);
  ICR1 = 1023 ; // 10   bit resolution
  OCR1A = 1; 
  

  Serial.println(TCCR1A,HEX);
  Serial.println(TCCR1B,HEX);
}

float filter(float y, float x)
{
  return 0.7*y+0.3*x;
}

float kp=5, ki=10; // kp=Gain, ki=Gain/Tau
float eprev=0;
uint32_t ty;

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

float u = 0, mv = 224;

void loop()
{
  int x1;
  float x2;
 
  if (Serial.available())
  switch(Serial.read()){
    case 'o':    x1 = Serial.parseInt();  Serial.print(":ocr");    Serial.println(x1);  mv=x1; break;
    case 'i':    x1 = Serial.parseInt();  Serial.print(":icr");    Serial.println(x1);  ICR1 =x1; break;
    case 'p':    x2 = Serial.parseFloat();  Serial.print(":kp");    Serial.println(x2);  kp=x2; break;
    case 't':    x2 = Serial.parseFloat();  Serial.print(":ki");    Serial.println(x2);  ki=x2; break;
    case '?':   
     Serial.print(":ocr");    Serial.println(OCR1A);
     Serial.print(":icr");    Serial.println(ICR1);
     Serial.print(":kp");    Serial.println(kp);
     Serial.print(":ki");    Serial.println(ki);
    break;
  }
  x1 = analogRead(A6);
  u = filter(u, 1.12*x1); // 2*5.1M+10k divider, 1.1v/1024
  mv += pi_ctrl(193, u);
  if (mv<0)   mv=0;
  if (mv>500) mv=500;
  OCR1A=(int)mv;
  if ((millis()&511)==0) { Serial.println(u); }
}

