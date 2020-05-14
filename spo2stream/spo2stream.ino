#include <Wire.h>
#include "MAX30105.h"

#define FreqS 25   //sampling frequency
#define led 13

void hr_spo2();
void sort_ascend(int *x, int n);
void stream_ir(int32_t ir, int32_t r);
void zlocs();
bool store_pulse(int32_t x, int32_t r, int32_t ir);
void calc_hr_spo2();

MAX30105 particleSensor;

void setup()
{
  Serial.begin(115200); // initialize serial communication at 115200 bits per second:
  pinMode(led,OUTPUT);

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println(F("MAX30105 was not found. Please check wiring/power."));
    while (1);
  }

  //  Serial.println(F("Attach sensor to finger with rubber band. Press any key to start conversion"));
  //  while (Serial.available() == 0) ; //wait until user presses a key
  //  Serial.read();

  byte ledBrightness = 30; //Options: 0=Off to 255=50mA
  byte sampleAverage = 1; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  byte sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 4096; //Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings
  zlocs();
  Serial.println("ired red");
}

#define FILTRZRO 0x80000000
#define FILTR100 16
//a=2Pi*Fcut*Ts/(1+2Pi*Fcut*Ts) n=1/a Ts=sampletime
//y=a*x+(1-a)y=ax+y-ay y+=a(x-y) ny+=(x-ny/n) init: ny=x*n
int32_t afilter(int32_t *y, int32_t x)
{
  if ((*(uint32_t*)y) == FILTRZRO) *y = ((int32_t)x) * FILTR100;
  (*y) -= (*y) / FILTR100;
  (*y) += x;
  return (*y) / FILTR100;
}

int32_t moveavg4pt(int32_t *y, int32_t x)
{
  int32_t tmp;
  y[0] = y[1]; y[1] = y[2]; y[2] = y[3]; y[3] = x;
  tmp = y[0] + y[1] + y[2] + y[3];
  return tmp / 4;
}

// bidirectional moving average
int32_t moveavg4ptbd(int32_t *y, int32_t x, int32_t *yr)
{
  int32_t tmp;
  y[0] = y[1];
  y[1] = y[2];
  y[2] = y[3];
  y[3] = y[4];
  y[4] = x;
  tmp = y[0] + y[1] + y[2] + y[2] + y[3] + y[4];
  *yr = y[2];
  return tmp / 6;
}

int32_t diff(int32_t *p, int32_t x)
{
  int32_t d;
  d = x - *p;
  *p = x;
  return d;
}

bool zcd(int32_t *p, int32_t x)
{
  bool z = 0;
  if (*p <= 0 && x > 0) z = 1;
  if (*p >= 0 && x < 0) z = 1;
  *p = x;
  return z;
}

int32_t fr = 0x80000000;
int32_t fi = 0x80000000;

int32_t mvr[7];
int32_t mvi[7];
int32_t pmi = 0, zmi = 0;
int32_t ci, cr, cx = 0;
int32_t pi, pr, px = 0;

void stream_ir(int32_t ir, int32_t r)
{
  int32_t dr, di;
  int32_t mi;

  px++;

  moveavg4ptbd(mvr, r, &dr);
  mi = moveavg4ptbd(mvi, ir, &di);
  if (zcd(&zmi, diff(&pmi, mi))) {
    ci = pi;
    cr = pr;
    cx = px;
    px = 0;
  }
  pi = di;
  pr = dr;
}

#define LOCS 15
int xloc[LOCS];
int32_t viloc[LOCS];
int32_t vrloc[LOCS];
void zlocs()
{
  for (int k = 0; k < LOCS; k++)
    xloc[k] = 0;
}

bool store_pulse(int32_t x, int32_t r, int32_t ir)
{
  bool pk;

  if (xloc[0] != 0 && xloc[1] != 0 && ir > viloc[0])
    pk = 1;
  else
    pk = 0;

  for (int k = LOCS - 1; k > 0; k--) {
    xloc[k] = xloc[k - 1];
    viloc[k] = viloc[k - 1];
    vrloc[k] = vrloc[k - 1];
  }
  xloc[0] = x;
  viloc[0] = ir;
  vrloc[0] = r;
  return pk;
}

bool calc_ac_dc_dt(int i, int32_t *y, int32_t *ac, int32_t *dc, int *dt)
{
  int x1=xloc[i];
  int xm=xloc[i+1];
  int x2=x1+xm;
  int32_t tmp;
  if (x1==0 || xm==0 || xloc[i+2]==0) return 0;
  if(dt) *dt = x2;
  tmp = (y[i+2]-y[i]) * x1;
  tmp = y[i]+tmp/x2; //  dc = y1+(xm-x1)*(y2-y1)/(x2-x1)
  if(dc) *dc = tmp;
  if(ac) *ac = tmp - y[i+1];
  return 1;
}

int ratio_avg(int *x, int n)
{
  int  middle = n / 2;

  sort_ascend(x, n);

  if (middle > 1)
    return (x[middle-1] + x[middle]) / 2; // use median
  else
    return x[middle];
}

int hr,spo2;
void calc_hr_spo2()
{
  int32_t irac,irdc,redac,reddc;
  int dt,n;
  int32_t interval_sum;
  int ratios[5];

  n=0;
  interval_sum=0;
  for (int k = 0; k < LOCS && n < 5; k+=2) {
    if (!calc_ac_dc_dt(k, viloc, &irac, &irdc, &dt)) break;
    if (!calc_ac_dc_dt(k, vrloc, &redac, &reddc, NULL)) break;
    int32_t nume = (redac * irdc) >> 7 ; //prepare X100 to preserve floating value
    int32_t denom = (irac * reddc) >> 7;
    if (denom > 0  &&  nume != 0) {
        //formular is (red_ac/red_dc) / (ir_ac/ir_dc) = (red_ac*ir_dc) / (ir_ac*red_dc)
        ratios[n++] = (nume * 100) / denom;
        interval_sum+=dt;
    }
  }
//  Serial.print("n=");
//  Serial.print(n);
  if (n < 2) return;
  hr = (int32_t) (FreqS * 60 * (int32_t)(n-1)) / interval_sum ;
  int ratio_average = ratio_avg(ratios, n);
  spo2=ratio_average;
/*
  Serial.print(" hr=");
  Serial.print(hr);
  Serial.print(" spo2rx100=");
  Serial.println(spo2);
  */
}

void loop()
{
  int32_t ir, r;
  while (particleSensor.available() == false) //do we have new data?
    particleSensor.check(); //Check the sensor for new data

  r = particleSensor.getRed();
  ir = particleSensor.getIR();
  particleSensor.nextSample(); //We're finished with this sample so move to next sample

  stream_ir(ir, r);
  if (cx > 0) {
/*    
    Serial.print("t=");
    Serial.print(cx);
    Serial.print(" r=");
    Serial.print(cr);
    Serial.print(" ir=");
    Serial.println(ci);
*/    
    if (store_pulse(cx, cr, ci)) {
      digitalWrite(led,1);
      int32_t iac,idc;
      int32_t rac,rdc;
      int dt;
      calc_ac_dc_dt(0, viloc, &iac, &idc, &dt);
      calc_ac_dc_dt(0, vrloc, &rac, &rdc, NULL);
      Serial.print("dt=");
      Serial.print(dt); Serial.print('i'); Serial.print(idc); Serial.print(' '); Serial.print(iac);
                        Serial.print('r'); Serial.print(rdc); Serial.print(' '); Serial.print(rac);
      Serial.println();

      calc_hr_spo2();
    } else
      digitalWrite(led,0);
    cx=0;
  }

/*
  Serial.print(ci);
  Serial.print(' ');
  Serial.println(pi);
  */
}

void sort_ascend(int *x, int n)
{
  // insertion sort
  for (int i = 1; i < n; i++) {
    int j = i, temp = x[i];
    for (; j > 0 && temp < x[j - 1]; j--)
      x[j] = x[j - 1];
    x[j] = temp;
  }
}
