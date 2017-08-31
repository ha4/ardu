#include <OneWire.h>

/*
DS18B20 Temperature chip.
 */
enum { DS_SKIPROM = 0xCC, DS_RD_SCRATCHPAD = 0xBE, DS_RUN_ADC = 0x44 };


long ref_time;
float t_read, t_set, u_out;
float x[3];
float K, Ti, Td;
float e,e1,e2;
OneWire  bus1(15);  // on pin PC1

void setup(void) {
  pinMode(14, OUTPUT);  digitalWrite(14, 0); // GND on PC0
  Serial.begin(57600);
  ref_time = millis();
  t_read=0;
  t_set = 45.0;
  K=200;
  Ti=800;
  Td=0;
  e1=e2=0;
  u_out = 40;
  e = -300;
  Serial.println(":start");
  analogWrite(5, 0);
}

int8_t tempr(void) {
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
    return 0.95*a + 0.05*b;
}

void tcloop()
{
    float e, u;
    float kp,ki,kd;

    // e = filter(e,t_set - t_read);
    e = t_set - t_read;
    /* diff: u - u1 = kp(e-e1) + ki*e + kd(e-2e1+e2) */
    /* prop: u = kp*e + ki*E + kd(e-e1) */
    /* intg: U = kp*E + ki*E2 + kd*e */
    
    Serial.print("#err");    Serial.print(e,4);
    Serial.print(" int");    Serial.println(e2,4);
    if(Ti==0) ki=0; else ki=K*1.5/Ti;
    kp=K;
    kd=K*Td/1.5;
    u = kp*e + ki*e2 + kd*(e-e1);
    if (u < 0) u=0;
    else if (u > 255) u=255;
    else e2 += e; // integrate
    e1=e; u_out=u;
}

void show_param()
{
Serial.print(":set");    Serial.print(t_set);
Serial.print(" :K");     Serial.print(K);
Serial.print(" :Ti");    Serial.print(Ti);
Serial.print(" :Td");    Serial.print(Td);
Serial.print(" :err");    Serial.print(t_set - t_read,4);
Serial.print(" :int");    Serial.println(e2,4);
}

void iloop(void)
{
  int x1;
  float xx; uint32_t xxx;
 
  if (!Serial.available()) return;
  switch(Serial.read()){
    case 'p':    x1 = Serial.parseInt();    Serial.print(":pwm");    Serial.println(x1);    analogWrite(5, x1);    break;
    case 's':    xx= Serial.parseFloat();  Serial.print(":set");    Serial.println(xx);  t_set = xx; break;
    case 'P':    xx= Serial.parseFloat();  Serial.print(":K");     Serial.println(xx);  K = xx;   break;
    case 'I':    xx= Serial.parseFloat();  Serial.print(":Ti");    Serial.println(xx);  Ti = xx;   break;
    case 'D':    xx= Serial.parseFloat();  Serial.print(":Td");    Serial.println(xx);  Td = xx;   break;
//    case 'r':    reg=Serial.parseInt();    Serial.print("reg");    Serial.print(reg);    break;
//    case 'c':    ancal=Serial.parseInt();    Serial.print("analog-calibration");    Serial.print(ancal);    break;
//    case 'l':    x=Serial.parseInt(); ee.l[x]=Serial.parseInt(); Serial.print("long"); Serial.print(x); Serial.print("="); Serial.print(ee.l[x]);    break;
//    case 'f':    x=Serial.parseInt(); ee.f[x]=Serial.parseFloat();Serial.print("float"); Serial.print(x); Serial.print("="); Serial.print(ee.f[x]);    break;
    case '?':  show_param();     break;
//    case 'Z':  default_param();    Serial.print("defaults-param");    break;
//    case 'W':  x=Serial.parseInt();    if (x==1) { Serial.print("write-param"); store_param(); }    break;
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
      tcloop();
      Serial.print(t_read); Serial.print(" ");
      Serial.print(u_out); Serial.print(" ");
      Serial.println("");
      analogWrite(5,u_out);
  }
  iloop();
}

