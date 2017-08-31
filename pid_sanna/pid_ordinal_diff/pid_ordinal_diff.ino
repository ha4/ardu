#include <OneWire.h>
#include <EEPROM.h>

/*
DS18B20 Temperature chip.
 */
enum { DS_SKIPROM = 0xCC, DS_RD_SCRATCHPAD = 0xBE, DS_RUN_ADC = 0x44 };



enum { pTset=0, pP, pI, pD, pOut, pT0, pR0, pBeta, PARMSF, pTst=0, PARMSL};

#define PARAMSZE (sizeof(struct paramstore)) 
struct paramstore  {
    uint16_t crc;
    float f[PARMSF];
    uint32_t l[PARMSL];
} ee;

long ref_time;
float t_get, t_read, t_set, u_out;
float x[3];
float K, Ti, Td;
float e,e1,e2;
float calT0,calR0,calBeta,calRref;
OneWire  bus1(15);  // on pin PC1

void setup(void) {
  pinMode(14, OUTPUT);  digitalWrite(14, 0); // GND on PC0
  analogReference(DEFAULT);
  analogRead(A6);

  Serial.begin(57600);
  ref_time = millis();
  default_param();
  load_param();
  t_read=t_set;
  Serial.println(":start");
  analogWrite(5, 0);
}

int8_t tempr(void)
{
  byte data[12];  float t;
  int8_t rc = 0;
  if (millis() <= ref_time) return rc;
  if (!bus1.reset())  { rc = -1; goto no_exist; }
  bus1.write(DS_SKIPROM);  bus1.write(DS_RD_SCRATCHPAD);  bus1.vread(data, 9);
  if (OneWire::crc8(data, 9)==0) {
      t = (data[0] + 256.0*data[1]) / 16.0;
      if (t < 85.0) { t_read = t; rc = 1; } else rc = -3;
  } else rc = -2;
  bus1.reset();  bus1.write(DS_SKIPROM); bus1.write(DS_RUN_ADC);  bus1.power(true);
no_exist:
  ref_time = millis()+1500;
  return rc;
}

int8_t Rtempr(int a) 
{
  float r;
  if (a==0 || a==1 || a==1023) return -3;
  r = 1026.82/(a+2.41)-1.0; // U0/U-1 = Rref/R
    Serial.print("#A6 ");    Serial.print(a);
    Serial.print(" ");    Serial.print(calRref);
    Serial.print(" ");    Serial.print(calRref/r);

  r*= calR0/calRref; // R0/R = R0/Rref * Rref/R
  r = log(r);
  r = 1.0 - r*calT0/calBeta;
  t_get = calT0/r - 273.15;
  return 1;
}


float Fn(float i)
{
    float e;
    if (i > 20) return 1;
    if (i <-20) return -1;
    e = exp(-i);
    return (1.0-e)/(1.0+e);
}

float prod(float *a, float *b)
{
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}

void prod(float *a, float b)
{
    a[0]*=b;    a[1]*=b;    a[2]*=b;
}

void prod(float *a, float *b, float c)
{
    a[0]=b[0]*c;    a[1]=b[1]*c;    a[2]=b[2]*c;
}

void incr(float *a, float *b)
{
    a[0]+=b[0];  a[1]+=b[1];  a[2]+=b[2];
}

void Norm(float *a)
{
    float u;
    u = prod(a,a);
    if (u <= 0) return;
    prod(a, 1.0/sqrt(u));
}

float filter(float a, float b)
{
    if ( a <= -300) return b;
    return 0.8*a + 0.2*b;
}

void tcloop(float t)
{
    float u;
    float kp,ki,kd;

    e = filter(e, t_set - t);
    // e = t_set - t;
    /* diff: u - u1 = kp(e-e1) + ki*e + kd(e-2e1+e2) */
    /* prop: u = kp*e + ki*E + kd(e-e1) */
    /* intg: U = kp*E + ki*E2 + kd*e */

    x[0]=e-e1;
    x[1]=e;
    x[2]=e-2*e1+e2;
    
    if(Ti==0) ki=0; else ki=K*1.5/Ti;
    kp=K-ki/2;
    kd=K*Td/1.5;
    u = kp*x[0] + ki*x[1] + kd*x[2];
    
    Serial.print("#x");    Serial.print(x[0],4);
    Serial.print(" ");    Serial.print(x[1],4);
    Serial.print(" ");    Serial.print(x[2],4);
    Serial.print(" #y");    Serial.println(u,4);
    u = u + u_out;
    if (u < 0) u=0;
    else if (u > 255) u=255;
    e2=e1; e1=e; u_out=u;
}

void default_param()
{
  t_set = 45.0;
  K=200;
  Ti=800;
  Td=0;
  e1=e2=0;
  u_out = 129;
  e = -300;
  calT0=318.17,calR0=3105,calBeta=4221.26,calRref=3406;
}

void store_param()
{
    int i;
    struct paramstore tee;

  tee.f[pTset] = t_set;
  tee.f[pP]=K;
  tee.f[pI]=Ti;
  tee.f[pD]=Td;
  tee.f[pOut]=u_out;
  tee.f[pT0]=calT0;
  tee.f[pR0]=calR0;
  tee.f[pBeta]=calBeta;
    tee.crc = crc16(2+(uint8_t *)&tee, PARAMSZE-2);
    for(i=0; i < PARAMSZE; i++)
      EEPROM.write(i, ((uint8_t *)&tee)[i]);
}

uint8_t load_param()
{
    int i;
    uint16_t c1;
    struct paramstore tee;

    for(i=0; i < PARAMSZE; i++)
      ((uint8_t *)&tee)[i] = EEPROM.read(i);

    c1= crc16(2+(uint8_t *)&tee, PARAMSZE-2);
    if (tee.crc != c1)
        return 0;

  t_set = tee.f[pTset];
  K=tee.f[pP];
  Ti=tee.f[pI];
  Td=tee.f[pD];
  u_out = tee.f[pOut];
  calT0=tee.f[pT0];
  calR0=tee.f[pR0];
  calBeta=tee.f[pBeta];
  e = -300;

    return 1;
}

void show_param()
{
Serial.print(":set");    Serial.print(t_set);
Serial.print(" :P");     Serial.print(K);
Serial.print(" :Ti");    Serial.print(Ti);
Serial.print(" :Td");    Serial.print(Td);
Serial.print(" :err");    Serial.print(t_set - t_read,4);
Serial.print(" :eflt");    Serial.println(e,4);
}

void iloop(void)
{
  int x1;
  float xx; uint32_t xxx;
 
  if (!Serial.available()) return;
  switch(Serial.read()){
    case 'p':    x1 = Serial.parseInt();    Serial.print(":pwm");    Serial.println(x1);    analogWrite(5, x1);    break;
    case 's':    xx= Serial.parseFloat();  Serial.print(":set");    Serial.println(xx);  t_set = xx; break;
    case 'P':    xx= Serial.parseFloat();  Serial.print(":P");     Serial.println(xx);  K = xx;   break;
    case 'I':    xx= Serial.parseFloat();  Serial.print(":Ti");    Serial.println(xx);  Ti = xx;   break;
    case 'D':    xx= Serial.parseFloat();  Serial.print(":Td");    Serial.println(xx);  Td = xx;   break;
//    case 'r':    reg=Serial.parseInt();    Serial.print("reg");    Serial.print(reg);    break;
//    case 'c':    ancal=Serial.parseInt();    Serial.print("analog-calibration");    Serial.print(ancal);    break;
//    case 'l':    x=Serial.parseInt(); ee.l[x]=Serial.parseInt(); Serial.print("long"); Serial.print(x); Serial.print("="); Serial.print(ee.l[x]);    break;
//    case 'f':    x=Serial.parseInt(); ee.f[x]=Serial.parseFloat();Serial.print("float"); Serial.print(x); Serial.print("="); Serial.print(ee.f[x]);    break;
    case '?':  show_param();     break;
    case 'Z':  default_param();    Serial.println(":def-param");    break;
    case 'L':  load_param();    Serial.println(":load-param");    break;
    case 'W':  x1=Serial.parseInt();    if (x1==1) { Serial.println(":write-param"); store_param(); }    break;
  }
}

void loop(void) {
  int8_t r;
  r = tempr();
  if (r <= 0) { 
    switch(r) {
      case -1: Serial.println(":reset-error"); break;
      case -2: Serial.println(":crc-error"); break;
      case -3: Serial.println(":tmp-error"); break;
    }
  } else {
      Rtempr(analogRead(A6));
      tcloop(t_get);
      Serial.print(t_read); Serial.print(" ");
      Serial.print(t_get); Serial.print(" ");
      Serial.print(u_out); Serial.print(" ");
      Serial.println("");
      analogWrite(5,u_out);
  }
  iloop();
}

/* CRC16 Definitions */
static const uint16_t crc_table[16] = {
  0x0000, 0xcc01, 0xd801, 0x1400, 0xf001, 0x3c00, 0x2800, 0xe401,
  0xa001, 0x6c00, 0x7800, 0xb401, 0x5000, 0x9c01, 0x8801, 0x4400
};

uint16_t crc16(uint8_t *message, uint8_t length) 
{ 
  uint16_t crc; 
  uint8_t i; 

  crc=0xFFFF;
  for(i = 0; i != length; i++) 
    { 
	  crc ^= message[i];
	  crc = (crc >> 4) ^ crc_table[crc & 0xf];
	  crc = (crc >> 4) ^ crc_table[crc & 0xf];
    } 
  return crc; 
}

