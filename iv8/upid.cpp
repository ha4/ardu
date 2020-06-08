#include "upid.h"

void uPID::pid(float pv_)
{
  float e,de,dde;

  pv = pv_;
  e = sp - pv;
  de=e-eprev;
  dde=eprev2-2*eprev+e;
  eprev2=eprev;
  eprev=e;

  if(enabled)
    mv += kp*de+ki*e*dt+kd*e/dt;
  if (mv < mvm) mv=mvm;
  if (mv > mvM) mv=mvM;
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


