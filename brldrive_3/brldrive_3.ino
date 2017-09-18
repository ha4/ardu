enum { 
  motorPin1=9,  motorPin2=10, motorPin3=11,  motorPinSync=12,
};


int pwmSin[] = {
  127,110, 94, 78, 64, 50, 37, 26, 17, 10,  4,  1,
    0,  1,  4, 10, 17, 26, 37, 50, 64, 78, 94,110,
  127,144,160,176,191,204,217,228,237,244,250,253,
  254,253,250,244,237,228,217,204,191,176,160,144
}; // array of PWM duty values for 8-bit timer - sine function

/*
int pwmSin[]={
 511, 444, 379, 315, 256, 200, 150, 106,  68,  39,  17,   4,
 0,   4,  17,  39,  68, 106, 150, 200, 256, 315, 379, 444,
 511, 578, 643, 707, 767, 822, 872, 916, 954, 983,1005,1018,
 1022,1018,1005, 983, 954, 916, 872, 822, 767, 707, 643, 578
 }; // array of PWM duty values for 10-bit timer - sine function
 */

int increment = 1; // direction 
long motorDelayActual = 500;    // speed

int currentStepA=0;
int currentStepB=16;
int currentStepC=32;

long lastMotorDelayTime = 0;

void setup() {

  TCCR1B = TCCR1B & 0b11111000 | 0x01; // set PWM frequency @ 31250 Hz for Pins 9 and 10
  TCCR2B = TCCR2B & 0b11111000 | 0x01; // set PWM frequency @ 31250 Hz for Pins 11 and 3 (3 not used)
  ICR1 = 255 ; // 8 bit resolution
  //ICR1 = 1023 ; // 10 bit resolution


  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPinSync, OUTPUT);
}

int limit(int x, int lim)
{
  if (x >= lim) return x-lim;
  if (x <0) return x+lim;
  return x;
}

void loop()
{
  if((micros() - lastMotorDelayTime) <  motorDelayActual) return;

  lastMotorDelayTime = micros();

  if (currentStepA==0)  digitalWrite(motorPinSync, 1); 
  else digitalWrite(motorPinSync, 0);

  analogWrite(motorPin1, pwmSin[currentStepA]);
  analogWrite(motorPin2, pwmSin[currentStepB]);
  analogWrite(motorPin3, pwmSin[currentStepC]);
  currentStepA = limit(currentStepA + increment, 48);
  currentStepB = limit(currentStepB + increment, 48);
  currentStepC = limit(currentStepC + increment, 48);
}





