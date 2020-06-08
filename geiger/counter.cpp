
#include "geiger.h"

byte volatile ir;
unsigned long volatile it1, it2, it0;
float volatile mtime=NAN;

uint32_t volatile i_pulse=0;
uint32_t volatile rcpm=0;
unsigned long volatile tcpm=0;
uint32_t cpmbin[6]={0,0,0,0,0,0};
byte binptr=0;

void gm_pulse()
{
    ir = 1;
    it2 = it1;
    it1 = micros();
    i_pulse++;
}

float filter(float y, float x, float a)
{
  if (isnan(y)) return x;
  else return (1-a)*y+a*x;
}

/*
#define PULSES_SZ 20
int n_pulses=0, i_pulses=0;
uint32_t pulses[PULSES_SZ];
uint32_t pulsesum=0;

float pulse_new(uint32_t dt)
{
  if (n_pulses < PULSES_SZ) {
    pulses[n_pulses] = dt;
    n_pulses++;
  } else {
    pulsesum -= pulses[i_pulses];
    pulses[i_pulses] = dt;
    i_pulses++;
    if (i_pulses >= PULSES_SZ) i_pulses -= PULSES_SZ;
  }
  pulsesum += dt;
  cpm++;
  
  return 60e6/(pulsesum/n_pulses);
}
*/

void gm_intervals()
{
    unsigned long volatile _it1, _it2;

    digitalWrite(LED_PIN, 1);
    noInterrupts();
    _it1=it1; _it2=it2;
    interrupts();
    _it2 = _it1 - _it2;
    mtime = filter(mtime, _it2, 0.1);
    DEBUG(_it2);
    DEBUG('\n');
    digitalWrite(LED_PIN, 0);
}


void gm_counter()
{
  uint32_t c;
  c = i_pulse; i_pulse=0;
  rcpm += c;
  rcpm -= cpmbin[binptr];
  cpmbin[binptr]=c;
  if (++binptr > 5) binptr=0;
}

/*
http://www.muonhunter.com/blog/category/raspberry-pi/2  Calculating the dose on the SBM 20
    urph = icpm*60.0 * 0.28/30.0; // 0.28uR/s=30Hz CTC-5
    usvph= urph/102.0; // 102 R = 1Sv (gamma-, beta-)
    factor = 60 * 0.28/30 /102 = 0.00549
*/

float volatile cpmfactor=COUNTER_FACTOR;
int volatile use_rcpm=0;
int volatile use_cpm=0;
void gm_show()
{
    float icpm,usvph;

    icpm = 60e6/mtime; // counts per minute
    if (use_rcpm) usvph = cpmfactor * rcpm;
    else  usvph = icpm * cpmfactor;

#ifdef USE_DISPLAY
    if(use_cpm)
      counter_int(use_rcpm?rcpm:(uint16_t)icpm);
    else
      counter_value(usvph);
#endif

  DEBUG("icpm: ");
  DEBUG(icpm);
  DEBUG(" rcpm: ");
  DEBUG(rcpm);
  DEBUG(' ');

#ifdef USE_SERIAL
    MYSERIAL.print(usvph);
    MYSERIAL.println("uSv/h");
#endif
}

