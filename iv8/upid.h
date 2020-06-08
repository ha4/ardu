#ifndef __UPIDH__
#define __UPIDH__

#include "Arduino.h"

class uPID {
public:
  float mv, sp, pv;
  float kp,ki,kd; // kp=Gain, ki=Gain/Tau
  float dt;
  float eprev,eprev2;
  float mvm, mvM;
  bool enabled;

  uPID():mv(0),eprev(0),eprev2(0), mvm(0), mvM(1) { }
  void enable(bool en) { enabled = en; }
  void begin(float _kp, float _ki, float _kd, float _dt) {
    enable(1); mv=0; eprev=0; eprev2=0;
    ki=_ki, kd=_kd, kp=_kp, dt=_dt;
  }
  void end() { enable(0); }
  void limits(float minV, float maxV) { mvm=minV; mvM=maxV; }
  void pid(float pv_); // Dout = kp*Derr + ki*err*dt
  float MV() { return mv; }
  void  set(float sp_) { sp=sp_; }
};

class uAfilter {
public:
  int32_t Fy, y;
  int32_t ra;
  uAfilter(int32_t reverse_a) { ra = reverse_a; Fy=0; y=0; }
  int16_t filter(int16_t x);
  int16_t filter025(int16_t x);
  int16_t filter0125(int16_t x);
};

#endif

