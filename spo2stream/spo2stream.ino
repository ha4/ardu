#include <Wire.h>
//#include "MAX30105.h"

#define FreqS 25   //sampling frequency
#define led 13

void hr_spo2();
void sort_ascend(int *x, int n);
void stream_ir(int32_t ir, int32_t r);
void zlocs();
bool store_pulse(int32_t x, int32_t r, int32_t ir);
void calc_hr_spo2();
void printNum(unsigned long n);
uint16_t bcd8(uint8_t b);
uint16_t bcd16(uint16_t w);
uint32_t bcd24(uint32_t w);
void printBcd(uint32_t n);

#define MAX30105_ADDRESS          0x57 //7-bit I2C Address

// Status Registers
static const uint8_t MAX30105_INTSTAT1 =    0x00;
static const uint8_t MAX30105_INTSTAT2 =    0x01;
static const uint8_t MAX30105_INTENABLE1 =    0x02;
static const uint8_t MAX30105_INTENABLE2 =    0x03;

// FIFO Registers
static const uint8_t MAX30105_FIFOWRITEPTR =  0x04;
static const uint8_t MAX30105_FIFOOVERFLOW =  0x05;
static const uint8_t MAX30105_FIFOREADPTR =   0x06;
static const uint8_t MAX30105_FIFODATA =    0x07;

// Configuration Registers
static const uint8_t MAX30105_FIFOCONFIG =    0x08;
static const uint8_t MAX30105_MODECONFIG =    0x09;
static const uint8_t MAX30105_PARTICLECONFIG =  0x0A;    // Note, sometimes listed as "SPO2" config in datasheet (pg. 11)
static const uint8_t MAX30105_LED1_PULSEAMP =   0x0C;
static const uint8_t MAX30105_LED2_PULSEAMP =   0x0D;
static const uint8_t MAX30105_LED3_PULSEAMP =   0x0E;
static const uint8_t MAX30105_LED_PROX_AMP =  0x10;
static const uint8_t MAX30105_MULTILEDCONFIG1 = 0x11;
static const uint8_t MAX30105_MULTILEDCONFIG2 = 0x12;

// Die Temperature Registers
static const uint8_t MAX30105_DIETEMPINT =    0x1F;
static const uint8_t MAX30105_DIETEMPFRAC =   0x20;
static const uint8_t MAX30105_DIETEMPCONFIG =   0x21;

// Proximity Function Registers
static const uint8_t MAX30105_PROXINTTHRESH =   0x30;

// Part ID Registers
static const uint8_t MAX30105_REVISIONID =    0xFE;
static const uint8_t MAX30105_PARTID =      0xFF;    // Should always be 0x15. Identical to MAX30102.

// MAX30105 Commands
// Interrupt configuration (pg 13, 14)
static const uint8_t MAX30105_INT_A_FULL_MASK =   (byte)~0b10000000;
static const uint8_t MAX30105_INT_A_FULL_ENABLE =   0x80;
static const uint8_t MAX30105_INT_A_FULL_DISABLE =  0x00;

static const uint8_t MAX30105_INT_DATA_RDY_MASK = (byte)~0b01000000;
static const uint8_t MAX30105_INT_DATA_RDY_ENABLE = 0x40;
static const uint8_t MAX30105_INT_DATA_RDY_DISABLE = 0x00;

static const uint8_t MAX30105_INT_ALC_OVF_MASK = (byte)~0b00100000;
static const uint8_t MAX30105_INT_ALC_OVF_ENABLE =  0x20;
static const uint8_t MAX30105_INT_ALC_OVF_DISABLE = 0x00;

static const uint8_t MAX30105_INT_PROX_INT_MASK = (byte)~0b00010000;
static const uint8_t MAX30105_INT_PROX_INT_ENABLE = 0x10;
static const uint8_t MAX30105_INT_PROX_INT_DISABLE = 0x00;

static const uint8_t MAX30105_INT_DIE_TEMP_RDY_MASK = (byte)~0b00000010;
static const uint8_t MAX30105_INT_DIE_TEMP_RDY_ENABLE = 0x02;
static const uint8_t MAX30105_INT_DIE_TEMP_RDY_DISABLE = 0x00;

static const uint8_t MAX30105_SAMPLEAVG_MASK =  (byte)~0b11100000;
static const uint8_t MAX30105_SAMPLEAVG_1 =   0x00;
static const uint8_t MAX30105_SAMPLEAVG_2 =   0x20;
static const uint8_t MAX30105_SAMPLEAVG_4 =   0x40;
static const uint8_t MAX30105_SAMPLEAVG_8 =   0x60;
static const uint8_t MAX30105_SAMPLEAVG_16 =  0x80;
static const uint8_t MAX30105_SAMPLEAVG_32 =  0xA0;

static const uint8_t MAX30105_ROLLOVER_MASK =   0xEF;
static const uint8_t MAX30105_ROLLOVER_ENABLE = 0x10;
static const uint8_t MAX30105_ROLLOVER_DISABLE = 0x00;

static const uint8_t MAX30105_A_FULL_MASK =   0xF0;

// Mode configuration commands (page 19)
static const uint8_t MAX30105_SHUTDOWN_MASK =   0x7F;
static const uint8_t MAX30105_SHUTDOWN =    0x80;
static const uint8_t MAX30105_WAKEUP =      0x00;

static const uint8_t MAX30105_RESET_MASK =    0xBF;
static const uint8_t MAX30105_RESET =       0x40;

static const uint8_t MAX30105_MODE_MASK =     0xF8;
static const uint8_t MAX30105_MODE_REDONLY =  0x02;
static const uint8_t MAX30105_MODE_REDIRONLY =  0x03;
static const uint8_t MAX30105_MODE_MULTILED =   0x07;

// Particle sensing configuration commands (pgs 19-20)
static const uint8_t MAX30105_ADCRANGE_MASK =   0x9F;
static const uint8_t MAX30105_ADCRANGE_2048 =   0x00;
static const uint8_t MAX30105_ADCRANGE_4096 =   0x20;
static const uint8_t MAX30105_ADCRANGE_8192 =   0x40;
static const uint8_t MAX30105_ADCRANGE_16384 =  0x60;

static const uint8_t MAX30105_SAMPLERATE_MASK = 0xE3;
static const uint8_t MAX30105_SAMPLERATE_50 =   0x00;
static const uint8_t MAX30105_SAMPLERATE_100 =  0x04;
static const uint8_t MAX30105_SAMPLERATE_200 =  0x08;
static const uint8_t MAX30105_SAMPLERATE_400 =  0x0C;
static const uint8_t MAX30105_SAMPLERATE_800 =  0x10;
static const uint8_t MAX30105_SAMPLERATE_1000 = 0x14;
static const uint8_t MAX30105_SAMPLERATE_1600 = 0x18;
static const uint8_t MAX30105_SAMPLERATE_3200 = 0x1C;

static const uint8_t MAX30105_PULSEWIDTH_MASK = 0xFC;
static const uint8_t MAX30105_PULSEWIDTH_69 =   0x00;
static const uint8_t MAX30105_PULSEWIDTH_118 =  0x01;
static const uint8_t MAX30105_PULSEWIDTH_215 =  0x02;
static const uint8_t MAX30105_PULSEWIDTH_411 =  0x03;

//Multi-LED Mode configuration (pg 22)
static const uint8_t MAX30105_SLOT1_MASK =    0xF8;
static const uint8_t MAX30105_SLOT2_MASK =    0x8F;
static const uint8_t MAX30105_SLOT3_MASK =    0xF8;
static const uint8_t MAX30105_SLOT4_MASK =    0x8F;

static const uint8_t SLOT_NONE =        0x00;
static const uint8_t SLOT_RED_LED =       0x01;
static const uint8_t SLOT_IR_LED =        0x02;
static const uint8_t SLOT_GREEN_LED =       0x03;
static const uint8_t SLOT_NONE_PILOT =      0x04;
static const uint8_t SLOT_RED_PILOT =     0x05;
static const uint8_t SLOT_IR_PILOT =      0x06;
static const uint8_t SLOT_GREEN_PILOT =     0x07;

static const uint8_t MAX_30105_EXPECTEDPARTID = 0x15;

uint8_t readRegister8(uint8_t reg) {
  Wire.beginTransmission(MAX30105_ADDRESS);
  Wire.write(reg);
  Wire.endTransmission(false);

  Wire.requestFrom(MAX30105_ADDRESS, 1); // Request 1 byte
  if (Wire.available())
    return(Wire.read());
  return 0; //Fail
}

uint8_t read_reg(uint8_t reg) {
  Wire.beginTransmission(I2C_WRITE_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);

  Wire.requestFrom(I2C_READ_ADDR, 1); // Request 1 byte
//  if (Wire.available())
    return Wire.read();
//  return 0; //Fail
}

void writeRegister8(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MAX30105_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

void bitMask(uint8_t reg, uint8_t mask, uint8_t thing)
{
  uint8_t originalContents = read_reg(reg);
  originalContents = originalContents & mask;
  writeRegister8(reg, originalContents | thing);
}

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
  #define I2C_BUFFER_LENGTH BUFFER_LENGTH
#elif defined(__SAMD21G18A__)
  #define I2C_BUFFER_LENGTH SERIAL_BUFFER_SIZE
#else
  #define I2C_BUFFER_LENGTH 32
#endif

#define STORAGE_SIZE 4 //Each long is 4 bytes so limit this to fit on your micro
uint32_t sens_red[STORAGE_SIZE];
uint32_t sens_IR[STORAGE_SIZE];
byte sens_head;
byte sens_tail;
byte sens_activeLEDs;

void sens_softReset(void) {
  bitMask(MAX30105_MODECONFIG, MAX30105_RESET_MASK, MAX30105_RESET);
  unsigned long startTime = millis();
  while (millis() - startTime < 100) {
    uint8_t response = read_reg(MAX30105_MODECONFIG);
    if ((response & MAX30105_RESET) == 0) break;
    delay(1);
  }
}

void sens_clearFIFO(void) {
  writeRegister8(MAX30105_FIFOWRITEPTR, 0);
  writeRegister8(MAX30105_FIFOOVERFLOW, 0);
  writeRegister8(MAX30105_FIFOREADPTR, 0);
}

void sens_enableSlot(uint8_t slotNumber, uint8_t device) {
  switch (slotNumber) {
    case 1: bitMask(MAX30105_MULTILEDCONFIG1, MAX30105_SLOT1_MASK, device); break;
    case 2: bitMask(MAX30105_MULTILEDCONFIG1, MAX30105_SLOT2_MASK, device << 4); break;
    case 3: bitMask(MAX30105_MULTILEDCONFIG2, MAX30105_SLOT3_MASK, device); break;
    case 4: bitMask(MAX30105_MULTILEDCONFIG2, MAX30105_SLOT4_MASK, device << 4); break;
  }
}

void setup()
{
  Serial.begin(250000); // initialize serial communication at 115200 bits per second:
  pinMode(led,OUTPUT);

  Wire.begin();
  Wire.setClock(400000);
  if (read_reg(MAX30105_PARTID)!=MAX_30105_EXPECTEDPARTID) {
    Serial.println("No device");
    return;
  }
  //revisionID = 
  read_reg(MAX30105_REVISIONID);

  sens_softReset(); //Reset all configuration, threshold, and data registers to POR values
  bitMask(MAX30105_FIFOCONFIG, MAX30105_SAMPLEAVG_MASK, MAX30105_SAMPLEAVG_2);
  //  bitMask(MAX30105_FIFOCONFIG, MAX30105_A_FULL_MASK, 2); //Set to 30 samples to trigger an 'Almost Full' interrupt
  bitMask(MAX30105_FIFOCONFIG, MAX30105_ROLLOVER_MASK, MAX30105_ROLLOVER_ENABLE);
  sens_activeLEDs = 2;
  if (sens_activeLEDs == 3) bitMask(MAX30105_MODECONFIG, MAX30105_MODE_MASK, MAX30105_MODE_MULTILED); //Watch all three LED channels
  else if (sens_activeLEDs == 2) bitMask(MAX30105_MODECONFIG, MAX30105_MODE_MASK, MAX30105_MODE_REDIRONLY); //Red and IR
  else bitMask(MAX30105_MODECONFIG, MAX30105_MODE_MASK, MAX30105_MODE_REDONLY); //Red only
  bitMask(MAX30105_PARTICLECONFIG, MAX30105_ADCRANGE_MASK, MAX30105_ADCRANGE_4096); //15.63pA per LSB
  bitMask(MAX30105_PARTICLECONFIG, MAX30105_SAMPLERATE_MASK, MAX30105_SAMPLERATE_200);
  bitMask(MAX30105_PARTICLECONFIG, MAX30105_PULSEWIDTH_MASK, MAX30105_PULSEWIDTH_411); //18 bit resolution
  //Default is 0x1F which gets us 6.4mA
  //powerLevel = 0x02, 0.4mA - Presence detection of ~4 inch
  //powerLevel = 0x1F, 6.4mA - Presence detection of ~8 inch
  //powerLevel = 0x7F, 25.4mA - Presence detection of ~8 inch
  //powerLevel = 0xFF, 50.0mA - Presence detection of ~12 inch
  byte powerLevel = 50; //Options: 0=Off to 255=50mA
  writeRegister8(MAX30105_LED1_PULSEAMP, powerLevel);
  writeRegister8(MAX30105_LED2_PULSEAMP, powerLevel);
  writeRegister8(MAX30105_LED3_PULSEAMP, powerLevel);
  writeRegister8(MAX30105_LED_PROX_AMP, powerLevel);
  sens_enableSlot(1, SLOT_RED_LED);
  if (sens_activeLEDs > 1) sens_enableSlot(2, SLOT_IR_LED);
  if (sens_activeLEDs > 2) sens_enableSlot(3, SLOT_GREEN_LED);
  //enableSlot(1, SLOT_RED_PILOT); enableSlot(2, SLOT_IR_PILOT); enableSlot(3, SLOT_GREEN_PILOT);
  sens_clearFIFO(); //Reset the FIFO before we begin checking the sensor

//  zlocs();
  Serial.println("ired red");
}

uint32_t read3()
{
  byte temp[4];
  uint32_t *tempLong=(uint32_t*)&temp[0];

  temp[3] = 0;
  temp[2] = Wire.read();
  temp[1] = Wire.read();
  temp[0] = Wire.read();

  return (*tempLong) & 0x3FFFF;
}

// more/less correct read
void loop()
{
  uint32_t ir, r;
  while (sens_head == sens_tail) { //do we have new data?
    //Check the sensor for new data
    byte rp = read_reg(MAX30105_FIFOREADPTR);
    byte wp = read_reg(MAX30105_FIFOWRITEPTR);

    int n = 0;

    if (rp == wp) continue;
    n = wp - rp;
    if (n < 0) n += 32;
    int toRead = n * sens_activeLEDs * 3;

    Wire.beginTransmission(MAX30105_ADDRESS);
    Wire.write(MAX30105_FIFODATA);
    Wire.endTransmission();

    while (toRead > 0) {
      int toGet = toRead;
      if (toGet > I2C_BUFFER_LENGTH) toGet = I2C_BUFFER_LENGTH - (I2C_BUFFER_LENGTH % (sens_activeLEDs * 3));
      toRead -= toGet;

      Wire.requestFrom(MAX30105_ADDRESS, toGet);
      while (toGet > 0) {
        sens_head++; //Advance the head of the storage struct
        sens_head %= STORAGE_SIZE; //Wrap condition
        sens_red[sens_head] = read3();
        if (sens_activeLEDs > 1) sens_IR[sens_head] = read3();
        if (sens_activeLEDs > 2) read3();
        toGet -= sens_activeLEDs * 3;
      }
    } 
  }

  r = sens_red[sens_tail];
  ir = sens_IR[sens_tail];
  sens_tail++;sens_tail %= STORAGE_SIZE;

  printBcd(bcd24(ir));
  Serial.write(' ');
  printBcd(bcd24(r));
  Serial.write('\r');
  Serial.write('\n');

}

void loop1()
{
  static uint32_t t0;
  uint32_t t;
  t=micros();
  if (t-t0<10000) // 10 000 us = 10 ms == 100Hz
    return;
  t0=t;
  Serial.write("1 1\r\n",5);
}

int32_t fr = 0x80000000;
int32_t fi = 0x80000000;

int32_t mvr[7];
int32_t mvi[7];
int32_t pmi = 0, zmi = 0;
int32_t ci, cr, cx = 0;
int32_t pi, pr, px = 0;

#define LOCS 15
int xloc[LOCS];
int32_t viloc[LOCS];
int32_t vrloc[LOCS];


void loop0()
{
  int32_t ir, r;
//  while (particleSensor.available() == false) //do we have new data?
//    particleSensor.check(); //Check the sensor for new data

//  r = particleSensor.getRed();
//  ir = particleSensor.getIR();
//  particleSensor.nextSample(); //We're finished with this sample so move to next sample

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

void printNum(unsigned long n)
{
  static char buf[12];
  char *str = &buf[sizeof(buf) - 1];
  byte n1=0;

  *str = '\0';
  do { char c = n % 10;  n /= 10;
    *--str = c + '0';
    n1++;
  } while(n);
  Serial.write((const uint8_t *)str,n1);
}

uint16_t bcd8(uint8_t b) //bcd8 20us bcd16 52us bcd32 300us
{
  uint16_t c=0;
  for(byte n=0; n < 8; n++) {
   if ((c&0x0f) >= 0x05) c+=0x03;
   if ((c&0xf0) >= 0x50) c+=0x30;
   c<<=1; if(b&0x80) c++; b<<=1;
  }
  return c;
}

uint16_t bcd16(uint16_t w) // bcd16 52us
{
  uint16_t c=0;
  for(byte n=0; n < 16; n++) {
   if ((c&0xf) >= 0x5) c+=0x3;
   if ((c&0xf0) >= 0x50) c+=0x30;
   if ((c&0xf00) >= 0x500) c+=0x300;
   if ((c&0xf000) >= 0x5000) c+=0x3000;
   c<<=1; if(w&0x8000) c++; w<<=1;
  }
  return c;
}

uint32_t bcd24(uint32_t w) // bcd16 52us
{
  union { uint32_t c; uint8_t b[4]; } uc,uw;
  uc.c=0;
  uw.c=w;
  for(byte n=0; n < 24; n++) {
   if ((uc.b[0]&0xf) >= 0x5) uc.b[0]+=0x3;
   if ((uc.b[0]&0xf0)>= 0x50) uc.b[0]+=0x30;
   if ((uc.b[1]&0xf) >= 0x5) uc.b[1]+=0x3;
   if ((uc.b[1]&0xf0)>= 0x50) uc.b[1]+=0x30;
   if ((uc.b[2]&0xf) >= 0x5) uc.b[2]+=0x3;
   if ((uc.b[2]&0xf0)>= 0x50) uc.b[2]+=0x30;
   if ((uc.b[3]&0xf) >= 0x5) uc.b[3]+=0x3;
   if ((uc.b[3]&0xf0)>= 0x50) uc.b[3]+=0x30;
   uc.c<<=1; if(uw.b[2]&0x80) uc.c++; uw.c<<=1;
  }
  return uc.c;
}

void printBcd(uint32_t n) // conversion 32..8 us
{
  static char buf[10];
  char *str = &buf[sizeof(buf) - 1];

  *str = '\0';
  do { *--str = (n & 0xf) + '0'; n>>=4; } while(n);
  while(*str) Serial.write(*str++);
}
