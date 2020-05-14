#include <Wire.h>
#include "MAX30105.h"

#define BUF_PART_LOAD 25
#define FreqS 25    //sampling frequency
#define BUFFER_SIZE (FreqS * 4)
#define MA4_SIZE 4 // DONOT CHANGE
#define MAX_LOCS 15

void buffer_fifo(byte part);
void hr_spo2();
void _HR_and_SPO2(uint16_t *ir_buffer, uint16_t *red_buffer, int n, int *spo2, int8_t *svalid, int *hr, int8_t *hvalid);
uint16_t irBuffer[BUFFER_SIZE]; //infrared LED sensor data
uint16_t redBuffer[BUFFER_SIZE];  //red LED sensor data

MAX30105 particleSensor;

#define MAX_BRIGHTNESS 255

void setup()
{
  Serial.begin(115200); // initialize serial communication at 115200 bits per second:

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println(F("MAX30105 was not found. Please check wiring/power."));
    while (1);
  }

//  Serial.println(F("Attach sensor to finger with rubber band. Press any key to start conversion"));
//  while (Serial.available() == 0) ; //wait until user presses a key
//  Serial.read();

  byte ledBrightness = 60; //Options: 0=Off to 255=50mA
  byte sampleAverage = 2; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  byte sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 4096; //Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings
  buffer_fifo(BUF_PART_LOAD);
  buffer_fifo(BUF_PART_LOAD);
  buffer_fifo(BUF_PART_LOAD);
//  Serial.print("red i-red");
//  Serial.print("ir-filtr thr locs");
}

void loop()
{
//buffer length of 100 stores 4 seconds of samples running at 25sps
  buffer_fifo(BUF_PART_LOAD);
  hr_spo2();
//    stream_ir();
}

#define FILTR100 16
uint16_t afilter(int32_t *y, uint16_t x)
{ //a=2Pi*fc*Ts/(1+2Pi*fc*Ts) n=1/a 
  //y=a*x+(1-a)y=ax+y-ay y+=a(x-y) ny+=(x-ny/n) init: ny=x*n

  if ((*y)==0x80000000) *y=(int32_t)x*FILTR100;
  (*y)-=(*y)/FILTR100;
  (*y)+=x;
  return (*y)/FILTR100;
}

int32_t moveavg4pt(int32_t *y, int32_t x)
{
  int32_t tmp;
  y[0]=y[1]; y[1]=y[2]; y[2]=y[3]; y[3]=x;
  tmp=y[0]+y[1]+y[2]+y[3];
  return tmp/4;
}

int32_t fr=0x80000000;
int32_t fi=0x80000000;
int32_t mvr[4];
int32_t mvi[4];

void stream_ir()
{
  uint16_t r,ir;
  int32_t yr,yi;
  int32_t zr,zi;
  int32_t mr,mi;
  while (particleSensor.available() == false) //do we have new data?
  particleSensor.check(); //Check the sensor for new data

  r = particleSensor.getRed();
  ir = particleSensor.getIR();
  particleSensor.nextSample(); //We're finished with this sample so move to next sample

  yr=afilter(&fr,r);
  yi=afilter(&fi,ir);
  zr=-1*(r-yr);
  zi=-1*(ir-yi);
  mr=moveavg4pt(mvr,zr);
  mi=moveavg4pt(mvi,zi);
}

void hr_spo2()
{

  int    spo2; //SPO2 value
  int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
  int    heartRate; //heart rate value
  int8_t validHeartRate; //indicator to show if the heart rate calculation is valid

  _HR_and_SPO2(irBuffer, redBuffer, BUFFER_SIZE, &spo2, &validSPO2, &heartRate, &validHeartRate);

  Serial.print(F(", HR="));
  Serial.print(heartRate, DEC);

  Serial.print(F(", HRvalid="));
  Serial.print(validHeartRate, DEC);

  Serial.print(F(", SPO2="));
  Serial.print(spo2, DEC);

  Serial.print(F(", SPO2Valid="));
  Serial.println(validSPO2, DEC);
}

void fill_buffer(int i)
{
  while (particleSensor.available() == false) //do we have new data?
    particleSensor.check(); //Check the sensor for new data

  redBuffer[i] = particleSensor.getRed();
  irBuffer[i] = particleSensor.getIR();
  particleSensor.nextSample(); //We're finished with this sample so move to next sample
}

void buffer_fifo(byte part)
{
    for (byte i = part; i < BUFFER_SIZE; i++) {
      redBuffer[i-part]=redBuffer[i];
      irBuffer[i-part]=irBuffer[i];
    }
    for (byte i = BUFFER_SIZE-part; i < BUFFER_SIZE; i++) fill_buffer(i);
}

//uch_spo2_table is approximated as  -45.060*ratioAverage* ratioAverage + 30.354 *ratioAverage + 94.845 ;
const uint8_t spo2_table[184] PROGMEM = { 95, 95, 95, 96, 96, 96, 97, 97, 97, 97, 97, 98, 98, 98, 98, 98, 99, 99, 99, 99,
                                          99, 99, 99, 99, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
                                          100, 100, 100, 100, 99, 99, 99, 99, 99, 99, 99, 99, 98, 98, 98, 98, 98, 98, 97, 97,
                                          97, 97, 96, 96, 96, 96, 95, 95, 95, 94, 94, 94, 93, 93, 93, 92, 92, 92, 91, 91,
                                          90, 90, 89, 89, 89, 88, 88, 87, 87, 86, 86, 85, 85, 84, 84, 83, 82, 82, 81, 81,
                                          80, 80, 79, 78, 78, 77, 76, 76, 75, 74, 74, 73, 72, 72, 71, 70, 69, 69, 68, 67,
                                          66, 66, 65, 64, 63, 62, 62, 61, 60, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50,
                                          49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 31, 30, 29,
                                          28, 27, 26, 25, 23, 22, 21, 20, 19, 17, 16, 15, 14, 12, 11, 10, 9, 7, 6, 5,
                                          3, 2, 1
                                        } ;

int     npks, valley_locs[MAX_LOCS] ;

uint32_t xx_mean;
int xx(int *x)
{
    uint16_t *u=(uint16_t)x;
    int32_t tmp=0;
    tmp += -1 * (*u++ - xx_mean);
    tmp += -1 * (*u++ - xx_mean);
    tmp += -1 * (*u++ - xx_mean);
    tmp += -1 * (*u++ - xx_mean);
    return tmp/(int32_t)4;
}

int  _peaks_above_min_height(int *locs,  int  *x, int n, int min_height, int max_num);
int  _remove_close_peaks(int *locs, int npks, int min_distance);
void _sort_indices_descend(int  *x, int *indx, int n);
void _sort_ascend(int *x, int n);
int  _xxpeaks_above_min_height(int *locs,  int  *x, int n, int min_height, int max_num);
void _xxsort_indices_descend(int  *x, int *indx, int n);


uint32_t _calc_dc(uint16_t *ubuf, int n)
{
  uint32_t mean = 0;
  for (int k = 0 ; k < n; k++) mean += ubuf[k] ;
  return mean/n;
}

void _moving_average4pt(int *x, int n)
{
  for (int k = 0; k < n - MA4_SIZE; k++) {
    int32_t tmp = x[k] + x[k+1] + x[k+2] + x[k+3];
    x[k] = tmp / 4;
  }
}

int _calc_threshold(int *x, int n, int min_allowed, int max_allowed)
{
  int32_t th1 = 0;
  for (int k = 0 ; k < n; k++)
    th1 +=  x[k];
  th1 /=  n;
  if (th1 < min_allowed) th1 = min_allowed;
  if (th1 > max_allowed) th1 = max_allowed;
  return th1;
}

int _xxcalc_threshold(int *x, int n, int min_allowed, int max_allowed)
{
  int32_t th1 = 0;
  for (int k = 0 ; k < n; k++)
    th1 +=  xx(&x[k]);
  th1 /=  n;
  if (th1 < min_allowed) th1 = min_allowed;
  if (th1 > max_allowed) th1 = max_allowed;
  return th1;
}

void _null_data(int *locs, int n)
{
    for (int k = 0 ; k < n; k++) locs[k] = 0;
}

int32_t _interval_sum(int *locs, int n)
{
  int32_t sum=0;
  for (int k = 1; k < n; k++) sum += locs[k] - locs[k-1];
  return sum;
}

void _max_and_idx(uint16_t *x, int l, int lp, int *idx, int32_t *val)
{
      *val=-17000000; // minimal negavtive signed 24bit
      *idx=0;
      for (int i = l; i < lp; i++)
        if (x[i] > *val) {
          *val = x[i];
          *idx = i;
        }
}

int32_t _calc_ac(uint16_t *y, int x1, int x2, int pkidx)
{
  int32_t ac,y1,y2,ym;
  y1=y[x1]; // line by two points
  y2=y[x2];
  ym=y[pkidx];
  ac = (y2 - y1) * (pkidx - x1); // {parital slope}*{peak x offset}
  ac = y1 + ac / (x2 - x1); // {inter valley slope}*{peak x offset}
  ac = ym - ac;   // subracting linear DC compoenents from raw
  return ac;
}

int _ratio_avg(int *x, int n)
{
  int  middle = n / 2;

  _sort_ascend(x, n);

  if (middle > 1)
    return (x[middle-1] + x[middle]) / 2; // use median
  else
    return x[middle];
}


void _find_pulse(uint16_t *buf, int n)
{
//  int an_x[BUFFER_SIZE];

  _null_data(valley_locs, MAX_LOCS);

  uint32_t un_ir_mean = _calc_dc(buf, n);
  xx_mean = un_ir_mean;

  // remove DC and invert signal so that we can use peak detector as valley detector
//  for (int k = 0 ; k < n; k++ )
//    an_x[k] = -1 * (buf[k] - un_ir_mean) ;
//  _moving_average4pt(an_x, n);
  int *an_x=(int*)buf;
  
  int th1 = _xxcalc_threshold(an_x, n, 30, 60);
  // since we flipped signal, we use peak detector as valley detector
  npks = _xxpeaks_above_min_height(valley_locs, an_x, n, th1, MAX_LOCS);

  _xxsort_indices_descend(an_x, valley_locs, npks);  // Order peaks from large to small
  npks = _remove_close_peaks(valley_locs, npks, 4); // min distance = 4
  _sort_ascend(valley_locs, npks);  // Resort indices to ascending order
  npks = min(npks, MAX_LOCS);
}

void _HR_and_SPO2(uint16_t *ir_buffer, uint16_t *red_buffer, int n, int *spo2, int8_t *svalid, int *hr, int8_t *hvalid)
{
  uint16_t *an_x, *an_y;

  *hr = -999; // unable to calculate because # of peaks are too small
  *hvalid  = 0;

  if (ir_buffer == NULL) return;
  _find_pulse(ir_buffer, n);

  if (npks >= 2) {
    *hr = ( (FreqS * 60 * (npks-1)) / _interval_sum(valley_locs, npks) );
    *hvalid  = 1;
  }

  if (red_buffer == NULL) return;
  //  load raw value again for SPO2 calculation : RED(=y) and IR(=X)
  an_x =  ir_buffer;
  an_y =  red_buffer;

  *spo2 =  -999 ; // do not use SPO2 since valley loc is out of range
  *svalid  = 0;
  // find precise min near valley_locs
  for (int k = 0; k < npks; k++) if (valley_locs[k] >= n) return;

  //using exact_ir_valley_locs , find ir-red DC andir-red AC for SPO2 calibration an_ratio
  //finding AC/DC maximum of raw
  int an_ratio[5];
  int i_ratio_count = 0;
  _null_data(an_ratio, 5);
    
  // find max between two valley locations
  // and use an_ratio betwen AC compoent of Ir & Red and DC compoent of Ir & Red for SPO2
  for (int k = 0; k < npks - 1; k++) {
      int lp=valley_locs[k+1];
      int l=valley_locs[k];
      if (lp - l <= 3) continue;

      int x_dc_max_idx, y_dc_max_idx;
      int32_t x_dc_max, y_dc_max;
      // peak detect between valleys - value and index
      _max_and_idx(an_x, l, lp, &x_dc_max_idx, &x_dc_max);
      _max_and_idx(an_y, l, lp, &y_dc_max_idx, &y_dc_max);
      
      int32_t y_ac = _calc_ac(an_y, l, lp, y_dc_max_idx); //red
      int32_t x_ac = _calc_ac(an_x, l, lp, x_dc_max_idx); // ir

      int32_t nume = (y_ac * x_dc_max) >> 7 ; //prepare X100 to preserve floating value
      int32_t denom = (x_ac * y_dc_max) >> 7;
      if (i_ratio_count < 5 && denom > 0  &&  nume != 0) {
        //formular is (y_ac/y_dc_max) / (x_ac/x_dc_max) = (y_ac*x_dc_max) / (x_ac*y_dc_max)
        an_ratio[i_ratio_count++] = (nume * 100) / denom;
        i_ratio_count++;
      }
  }

  // choose median value since PPG signal may varies from beat to beat
  int ratio_average = _ratio_avg(an_ratio, i_ratio_count);
  if (ratio_average > 2 && ratio_average < 184) {
    *spo2 = pgm_read_byte_near(spo2_table + ratio_average);
    *svalid  = 1;//  float_SPO2 =  -45.060*n_ratio_average* n_ratio_average/10000 + 30.354 *n_ratio_average/100 + 94.845 ;  // for comparison with table
  }
}

int _peaks_above_min_height(int *locs,  int  *x, int n, int min_height, int max_num)
{
  int i = 1, width;
  int npks = 0;

  while (i < n - 1)
    if (x[i] > min_height && x[i] > x[i-1])     // find left edge of potential peaks
    {      
      for (width = 1; i + width < n && x[i] == x[i+width]; width++); // find flat peaks
      if (x[i] > x[i+width] && npks < max_num)   // find right edge of peaks
      {
        locs[npks] = i;
        npks++;
        i += width + 1; // for flat peaks, peak location is left edge
      } else
        i += width;
    } else
      i++;
  return npks;
}

int _xxpeaks_above_min_height(int *locs,  int  *x, int n, int min_height, int max_num)
{
  int i = 1, width;
  int npks = 0;

  while (i < n - 1) {
    int xi=xx(&x[i]);
    if (xi > min_height && xi > xx(&x[i-1]))     // find left edge of potential peaks
    {      
      for (width = 1; i + width < n && xi == xx(&x[i+width]); width++); // find flat peaks
      if (xi > xx(&x[i+width]) && npks < max_num)   // find right edge of peaks
      {
        locs[npks] = i;
        npks++;
        i += width + 1; // for flat peaks, peak location is left edge
      } else
        i += width;
    } else
      i++;
  } 
  return npks;
}

int _remove_close_peaks(int *locs, int npks, int min_distance)
{
  int old_npks, dist;

  for (int i = -1; i < npks; i++ ) {
    old_npks = npks;
    npks = i + 1;
    for (int j = i + 1; j < old_npks; j++ ) {
      dist =  locs[j] - (i == -1 ? -1 : locs[i] ); // lag-zero peak of autocorr is at index -1
      if (dist > min_distance || dist < -min_distance )
        locs[npks++] = locs[j];
    }
  }
  return npks;
}

void _sort_ascend(int *x, int n)
{ 
  // insertion sort
  for (int i = 1; i < n; i++) {
    int j = i, temp = x[i];
    for (; j > 0 && temp < x[j - 1]; j--)
      x[j] = x[j-1];
    x[j] = temp;
  }
}

void _sort_indices_descend(int  *x, int *indx, int n)
{
  for (int i = 1; i < n; i++) {
    int j = i, temp = indx[i];
    for (; j > 0 && x[temp] > x[indx[j-1]]; j--)
      indx[j] = indx[j-1];
    indx[j] = temp;
  }
}

void _xxsort_indices_descend(int  *x, int *indx, int n)
{
  for (int i = 1; i < n; i++) {
    int j = i, temp = indx[i];
    for (; j > 0 && xx(&x[temp]) > xx(&x[indx[j-1]]); j--)
      indx[j] = indx[j-1];
    indx[j] = temp;
  }
}
