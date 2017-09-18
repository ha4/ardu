// enum { motorPinAL=2,  motorPinAH=3,  motorPinBL=4,  motorPinBH=5,  motorPinCL=6,  motorPinCH=7,  motorPinSync=12, };

enum { 
  motorPinAH=3,
  motorPinAL=4,
  motorPinBH=18,
  motorPinBL=19,
  motorPinCH=6,
  motorPinCL=7,
  motorPinSync=12,
};

/*
0   180  360
~~~~~_____  HALL1
__~~~~~~__  HALL2
~_____~~~~  HALL3

-____-^^^^  A  aax---    AL-AX-AH
^^^^-____-  B  --bbx-    BH-BL-BX
__-^^^^-__  C  x---cc    CX-CH-CL
*/

/* deactivate/activate levels for high/low bridge transistor */
#define DEA_H 1
#define DEA_L 0
#define ACT_H 0
#define ACT_L 1

int h[]={ACT_H, ACT_H, DEA_H, DEA_H, DEA_H, DEA_H};
int l[]={DEA_L, DEA_L, DEA_L, ACT_L, ACT_L, DEA_L};

int increment = 1; // direction 
long motorDelayActual = 10000;    // speed

int currentStepA=0;
int currentStepB=2;
int currentStepC=4;

long lastMotorDelayTime = 0;
long lastMotorSpeedTime = 0;

void setup() {


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
}

int limit(int x, int lim)
{
  if (x >= lim) return x-lim;
  if (x <0) return x+lim;
  return x;
}
/*
void spinup()
{
  if((millis() - lastMotorSpeedTime) <  10) return;
  lastMotorSpeedTime = millis();
  if (motorSpeed < motorSpeedTarget) {
    motorSpeed+=1;
    motorDelayActual=1000000/motorSpeed;
  }
}
*/
void loop()
{
//  spinup();

  if((micros() - lastMotorDelayTime) <  motorDelayActual) return;
  lastMotorDelayTime = micros();


  if (currentStepA==0)  digitalWrite(motorPinSync, 1); 
  else digitalWrite(motorPinSync, 0);

  
  if (l[currentStepA]==DEA_L) digitalWrite(motorPinAL, DEA_L);  
  if (h[currentStepA]==DEA_H) digitalWrite(motorPinAH, DEA_H);
  if (l[currentStepB]==DEA_L) digitalWrite(motorPinBL, DEA_L); 
  if (h[currentStepB]==DEA_H) digitalWrite(motorPinBH, DEA_H); 
  if (l[currentStepC]==DEA_L) digitalWrite(motorPinCL, DEA_L);
  if (h[currentStepC]==DEA_H) digitalWrite(motorPinCH, DEA_H);

  delayMicroseconds(1);
  
  if (h[currentStepA]==ACT_H) digitalWrite(motorPinAH, ACT_H);
  if (l[currentStepA]==ACT_L) digitalWrite(motorPinAL, ACT_L);
  if (h[currentStepB]==ACT_H) digitalWrite(motorPinBH, ACT_H);
  if (l[currentStepB]==ACT_L) digitalWrite(motorPinBL, ACT_L);
  if (h[currentStepC]==ACT_H) digitalWrite(motorPinCH, ACT_H);
  if (l[currentStepC]==ACT_L) digitalWrite(motorPinCL, ACT_L);

  currentStepA = limit(currentStepA + increment, 6);
  currentStepB = limit(currentStepB + increment, 6);
  currentStepC = limit(currentStepC + increment, 6);
}





