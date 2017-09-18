enum {step=7, dir=8};

void setup()
{
  pinMode(dir,OUTPUT);
  pinMode(step,OUTPUT);
  Serial.begin(115200);
}

int flt=512*16;

void loop()
{
  int a,b;
  digitalWrite(step,1);
  delayMicroseconds(10);
  a = analogRead(A0);
  flt += a-(flt>>4);
  b=flt-512*16;
  digitalWrite(dir,(b<0)?1:0);
  digitalWrite(step,0);
  b=1000000L/((b<0)?-b:b);
//  Serial.println(b);
  delayMicroseconds(b);
}

