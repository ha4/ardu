#include "stepper.h"

enum { 
  motor_pin_1=8, motor_pin_2, motor_pin_3=3, motor_pin_4, 
  pwm_pin_a=10, pwm_pin_b=11, };
  

int STP_number, STP_speed, STP_step, STP_dir, STP_mA, STP_Gsens, STP_reverse;
int STP_npole, STP_usteps;
int running, run_amount;
long STP_time, STP_delay;
int number_of_steps;
uint8_t usin[17];

void Stepper_init()
{
  STP_number = 0;      // which step the motor is on
  STP_time = 0;    // time stamp in ms of the last step taken
  STP_dir = 0;     // 0,1,-1
  STP_reverse=0;
  running=0;
  run_amount=1;
  setSpeed(0,MOTOR_STEPS, MOTOR_uSTEPS);
  setCurrent(MOTOR_Ipk, 2); // milliAmpere, simens

  TCCR1B = TCCR1B & 0b11111000 | 0x01; // set PWM frequency @ 31250 Hz for Pins 9 and 10
  TCCR2B = TCCR2B & 0b11111000 | 0x01; // set PWM frequency @ 31250 Hz for Pins 11 and 3 (3 not used)
  ICR1 = 255 ; // 8 bit resolution

  pinMode(motor_pin_1, OUTPUT);
  pinMode(motor_pin_2, OUTPUT);
  pinMode(motor_pin_3, OUTPUT);
  pinMode(motor_pin_4, OUTPUT);
  analogWrite(pwm_pin_a, 0);
  analogWrite(pwm_pin_b, 0);
}

void setSpeed(int rph, int steps, int usteps)
{
  STP_npole = steps;
  STP_usteps = usteps;
  STP_speed = rph;

  number_of_steps = steps*usteps;
  STP_step=16/STP_usteps;
  STP_number=STP_step/2;

  STP_delay = 3600UL * 1000000UL / (number_of_steps) / rph;
}

void setCurrent(int mA, int sm) // milliAmpere, simens
{
  float pwmx;
  STP_mA = mA;
  STP_Gsens = sm;
  pwmx = 255./5000.*mA/sm;
  for(int a=0; a<=16; a++)
    usin[a]=(uint8_t)(pwmx*sin(3.1415926535*a/32.));
}

void stepMotor(int thisStep) // 0..63
{
  int u = thisStep&0x0F;
  switch (thisStep>>4) {
  case 0:    // 1010                A+B+
    digitalWrite(motor_pin_1, HIGH);
    digitalWrite(motor_pin_2, LOW);
    digitalWrite(motor_pin_3, HIGH);
    digitalWrite(motor_pin_4, LOW);
    analogWrite(pwm_pin_a, usin[16-u]);
    analogWrite(pwm_pin_b, usin[u]);
    break;
  case 1:    // 0110                A-B+
    digitalWrite(motor_pin_1, LOW);
    digitalWrite(motor_pin_2, HIGH);
    digitalWrite(motor_pin_3, HIGH);
    digitalWrite(motor_pin_4, LOW);
    analogWrite(pwm_pin_a, usin[u]);
    analogWrite(pwm_pin_b, usin[16-u]);
    break;
  case 2:    //0101                 A-B-
    digitalWrite(motor_pin_1, LOW);
    digitalWrite(motor_pin_2, HIGH);
    digitalWrite(motor_pin_3, LOW);
    digitalWrite(motor_pin_4, HIGH);
    analogWrite(pwm_pin_a, usin[16-u]);
    analogWrite(pwm_pin_b, usin[u]);
    break;
  case 3:    //1001                 A+B-
    digitalWrite(motor_pin_1, HIGH);
    digitalWrite(motor_pin_2, LOW);
    digitalWrite(motor_pin_3, LOW);
    digitalWrite(motor_pin_4, HIGH);
    analogWrite(pwm_pin_a, usin[u]);
    analogWrite(pwm_pin_b, usin[16-u]);
    break;
  } 
}

void rawStepper(int dir)
{
  if (STP_reverse) dir = -dir;
  switch(dir) {
  case 1:
    STP_number+=STP_step;
    if (STP_number >= number_of_steps) STP_number = 0;
    stepMotor(STP_number % 64);
    break;
  case -1:
    if (STP_number <= 0) STP_number = number_of_steps;
    STP_number-=STP_step;
    stepMotor(STP_number % 64);
    break;
  } 
}

int pulseStepper()
{  
  long now = micros();
  if (now - STP_time <= STP_delay) return 0;
  STP_time = now;
  rawStepper(STP_dir);
  return 1;
}

void stepStepper(int steps_to_move)
{  
  int steps_left = abs(steps_to_move);  // how many steps to take

  if (steps_to_move < 0) STP_dir=-1;
  if (steps_to_move > 0) STP_dir=1;

  while(steps_left > 0)
    if(pulseStepper()) steps_left--;
}

