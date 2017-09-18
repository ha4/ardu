/* pwm-ENABLED 3,   5,   6,   9,  10,  11  5,6-980Hz, other 490Hz
             pd3, pd5, pd6, pb1, pb2, pb3
            oc2b,oc0b,oc0a,oc1a,oc1b,oc2a
MODE: +PWM(act=0) -ON(act=1)
*/
enum { 
  motorPinAH=3,
  motorPinBH=18,
  motorPinCH=6,
  motorPinAL=4,
  motorPinBL=19,
  motorPinCL=7,
  motorPinSync=12,
};

/* deactivate/activate levels for high/low bridge transistor */
#define ACT_H 0
#define DEA_H 1
#define ACT_L 1
#define DEA_L 0

/*int pwmSin[] = {
  127,110, 94, 78, 64, 50, 37, 26, 17, 10,  4,  1,
    0,  1,  4, 10, 17, 26, 37, 50, 64, 78, 94,110,
  127,144,160,176,191,204,217,228,237,244,250,253,
  254,253,250,244,237,228,217,204,191,176,160,144
}; // array of PWM duty values for 8-bit timer - sine function
*/
uint8_t sinus8[65];
#define PERIOD 256
void sinus_setup(float amp)
{
  float f;
  for(int a=0; a <= 64; a++) {
    f=3.1415926535/128.0*a;
    f=sin(f);
//    f=1.1*sin(f)+0.1*sin(3*f);
//    if(f>0) f=sqrt(f);
//    f*=f;
//    f=a/64.0;
    sinus8[a]=128 + amp*f;
  }
}

uint8_t sinus256(uint8_t a)
{
if (a>=128) return 0-sinus256(a-128);
if (a<=64) return sinus8[a];
else return sinus8[128-a];
}

int increment = 1; // direction 
long motorDelayActual = 5000;    // speed
long motorSpeedTarget = 5;
long motorSpeed = 2;

uint8_t currentStepA=0;
uint8_t currentStepB=85;
uint8_t currentStepC=171;

long lastMotorDelayTime = 0;
long lastMotorSpeedTime = 0;

void setup() {
  Serial.begin(57600);
  sinus_setup(50);

  pinMode(motorPinAL, OUTPUT);
  pinMode(motorPinAH, OUTPUT);
  pinMode(motorPinBL, OUTPUT);
  pinMode(motorPinBH, OUTPUT);
  pinMode(motorPinCL, OUTPUT);
  pinMode(motorPinCH, OUTPUT);
  pinMode(motorPinSync, OUTPUT);
  digitalWrite(motorPinAH, DEA_H);
  digitalWrite(motorPinAL, DEA_L);
  digitalWrite(motorPinBH, DEA_H);
  digitalWrite(motorPinBL, DEA_L);
  digitalWrite(motorPinCH, DEA_H);
  digitalWrite(motorPinCL, DEA_L);
//  for(int a=0; a <= 256; a+=64) {
//    Serial.print("sin("); Serial.print(a); Serial.print(")=");  Serial.println(sinus256(a));
//  }
}

uint8_t pdmA=0, pdmB=0, pdmC=0;
uint8_t outA=0, outB=0, outC=0;
uint8_t genA=0, genB=0, genC=0;
void pulse()
{
  uint16_t z1;

  if (outA) digitalWrite(motorPinAL, DEA_L);  
       else digitalWrite(motorPinAH, DEA_H);
  if (outB) digitalWrite(motorPinBL, DEA_L); 
       else digitalWrite(motorPinBH, DEA_H); 
  if (outC) digitalWrite(motorPinCL, DEA_L);
       else digitalWrite(motorPinCH, DEA_H);

  delayMicroseconds(1);
  
  if (outA) digitalWrite(motorPinAH, ACT_H);
       else digitalWrite(motorPinAL, ACT_L);
  if (outB) digitalWrite(motorPinBH, ACT_H);
       else digitalWrite(motorPinBL, ACT_L);
  if (outC) digitalWrite(motorPinCH, ACT_H);
       else digitalWrite(motorPinCL, ACT_L);

  z1=pdmA+genA; pdmA=lowByte(z1); outA=highByte(z1);
  z1=pdmB+genB; pdmB=lowByte(z1); outB=highByte(z1);
  z1=pdmC+genC; pdmC=lowByte(z1); outC=highByte(z1);
}


void spinup()
{
  if((millis() - lastMotorSpeedTime) <  100) return;
  lastMotorSpeedTime = millis();
  if (motorSpeedTarget == 0) { brake(); return; }
  if (motorSpeed != motorSpeedTarget) {
    if (motorSpeed < motorSpeedTarget)
      motorSpeed++;
    else
      motorSpeed--;
    Serial.print(":speed");    Serial.println(motorSpeed);
    motorDelayActual=1000000/motorSpeed/PERIOD;
  }
}

void brake()
{
  outC=outB=outA=0;
  currentStepC = currentStepB = currentStepA = 0;
  motorDelayActual = 0xFFFFFFFF;
}

void brakeP()
{
  outC=outB=outA=1;
  currentStepC = currentStepB = currentStepA = 64;
  motorDelayActual = 0xFFFFFFFF;
}

void iloop(void)
{
  if (!Serial.available()) return;
  switch(Serial.read()){
    case 's':  motorSpeedTarget = Serial.parseInt();    Serial.print(":speed");    Serial.println(motorSpeedTarget);  break;
    case 'b':  brake();     Serial.println(":brake");    break;
    case 'B':  brakeP();    Serial.println(":brake+");    break;
    case '+':  increment = Serial.parseInt(); 
      currentStepA = currentStepA + increment;
      currentStepB = currentStepB + increment;
      currentStepC = currentStepC + increment;
      Serial.print(":phaze");    Serial.println(currentStepA);
      motorDelayActual = 0xFFFFFFFF; 
    break;
    case '-':  increment = -Serial.parseInt(); 
      currentStepA = currentStepA + increment;
      currentStepB = currentStepB + increment;
      currentStepC = currentStepC + increment;
      Serial.print(":phaze");    Serial.println(currentStepA);
      motorDelayActual = 0xFFFFFFFF; 
    break;
    case 'f':   currentStepA=Serial.parseInt();
                currentStepB=85+currentStepA;
                currentStepC=171+currentStepA;
                Serial.print(":phaze");    Serial.println(currentStepA);
                motorDelayActual = 0xFFFFFFFF; increment = 0;
                break;
  }
}


void rotor(void)
{
  if (currentStepA==0)  digitalWrite(motorPinSync, 1); 
  else digitalWrite(motorPinSync, 0);

  genA = sinus256(currentStepA);
  genB = sinus256(currentStepB);
  genC = sinus256(currentStepC);
  if ((micros() - lastMotorDelayTime) >= motorDelayActual) {
    lastMotorDelayTime = micros();
    currentStepA = currentStepA + increment;
    currentStepB = currentStepB + increment;
    currentStepC = currentStepC + increment;
  }
}

void loop()
{
  pulse();
  spinup();
  rotor();
  iloop();
}





