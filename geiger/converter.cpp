#include "geiger.h"

uint16_t  volatile setp = CONVERTER_SETPOINT;

float kp=5, ki=10; // kp=Gain, ki=Gain/Tau
float eprev=0;
uint32_t ty;
float volatile u = 0, ui = 0, mv = 224;

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

void ustab()
{
  int x1;

  x1 = analogRead(CONVERTER_U_PIN);
  u = filter(u, x1*CONVERTER_U_FACTOR, 0.3); // 9.1M+10k divider, 1.1v/1024
#ifdef CONVERTER_CURRENT
  x1 = analogRead(CONVERTER_I_PIN);
  ui = filter(ui, x1*CONVERTER_I_FACTOR, 0.3); // 100 Ohm, 1100mv/1024
#endif

#if !defined(CONVERTER_OPENLOOP)
  mv += pi_ctrl(setp, u);
  if (mv<0)   mv=0;
  if (mv>500) mv=500;
  OCR1A=(int)mv;
#endif
}

void ustab_show()
{ 
    DEBUG("mv:");
    DEBUG(mv);
    DEBUG(" u:");
    DEBUG(u);
#ifdef CONVERTER_CURRENT
    DEBUG(" i:");
    DEBUG(ui);
#endif
    DEBUG(' ');
}

