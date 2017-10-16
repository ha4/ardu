enum {pwmin=2,
ch_a=4, ch_b=5, ch_pwm=3};

int32_t fx;

void setup()
{
  pinMode(pwmin, INPUT);
  digitalWrite(ch_a, 0);  pinMode(ch_a, OUTPUT);
  digitalWrite(ch_b, 0);  pinMode(ch_b, OUTPUT);
  digitalWrite(ch_pwm, 0);  analogWrite(ch_pwm, 0);
  Serial.begin(115200);
  fx=0;
}

int16_t dfiltr(int32_t *y, int16_t x)
{
  (*y)+= (((int32_t)x*8) - *y)/8;
//  y' = y(1-a)+xa = y - ay + ax = y + ax-ay => y+= ax-ay, a'=1/a
//  ie:  a'y+=x-y/a'
  return (*y) / 8;
}

char bu[40];
void loop()
{
  int x,y,a,b;
  x = pulseIn(pwmin, HIGH, 20000);
  if (x!=0) x-=1500;
  x=dfiltr(&fx,x);
  y=(x<0)?-x:x;
  y-=y/3;
  if (y<0)y=0; if (y>255)y=255;
  
  a=b=0;
  if (y>20 && x<=0) a=1;
  if (y>20 && x>0) b=1;
  if (y<20) y=255;
  analogWrite(ch_pwm,y);
  digitalWrite(ch_a,a);
  digitalWrite(ch_b,b);
  sprintf(bu, "%5d %5d a%d b%d\n",x,y,a,b);
  Serial.print(bu);
}

