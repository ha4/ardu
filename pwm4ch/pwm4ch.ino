enum {ch1=17, ch2=16, ch3=15, ch4=14,
out1=13, out2=12, out3=11, out4=10};

void setup()
{
  pinMode(ch1, INPUT);
  pinMode(ch2, INPUT);
  pinMode(ch3, INPUT);
  pinMode(ch4, INPUT);
  digitalWrite(out1, 1);  pinMode(out1, OUTPUT);
  digitalWrite(out2, 1);  pinMode(out2, OUTPUT);
  digitalWrite(out3, 1);  pinMode(out3, OUTPUT);
  digitalWrite(out4, 1);  pinMode(out4, OUTPUT);
  Serial.begin(115200);
}

char bu[40];
void loop()
{
  int pwm1,pwm2,pwm3,pwm4;
  pwm1 = pulseIn(ch1, HIGH, 20000);
  pwm2 = pulseIn(ch2, HIGH, 20000);
  pwm3 = pulseIn(ch3, HIGH, 20000);
  pwm4 = pulseIn(ch4, HIGH, 20000);
  digitalWrite(out1, (pwm1 > 1600)?0:1);
  digitalWrite(out2, (pwm2 > 1600)?0:1);
  digitalWrite(out3, (pwm3 > 1600)?0:1);
  digitalWrite(out4, (pwm4 > 1600)?0:1);
  sprintf(bu, "%5d %5d %5d %5d\n",pwm1,pwm2,pwm3,pwm4);
  Serial.print(bu);
}

