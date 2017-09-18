#include <Arduino.h>
#include <avr/pgmspace.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

/* 
 * Enable EEPROM save over reprograming flash:
 *
 * C:\Program Files (x86)\Arduino\hardware/tools/avr/bin/avrdude 
 * -CC:\Program Files (x86)\Arduino\hardware/tools/avr/etc/avrdude.conf
 * -v -v -v -v -pattiny85 -cusbtiny -Pusb -e 
 * -Uefuse:w:0xff:m -Uhfuse:w:0xd7:m -Ulfuse:w:0xe2:m 
 */

byte eerd(int a) { 
  return EEPROM.read(a); 
}
void eewr(int a, byte d) { 
  EEPROM.write(a,d); 
}

SoftwareSerial mySerial(3, 2); // RX, TX
void sprint(const char *s) {   
  mySerial.print(s);  
}
void fprint(float m) { 
  mySerial.print(m); 
}

unsigned long ltime;
unsigned long tinterval;

float pidSP = 2844.3;
float Pb=-261,Ti=426,Td=0,Ts = 0.35;
float Rref = 3200.9;
float T0 = 318.15, R0=3324.04, beta=4221.26;

float pidPV=0, u=0;
float e,e1=0,e2=0;
float kp,ki,kd;

#define CRC_START 0xFFFF
uint16_t crc;

const char* parn[]  = {
  "Set", "span", "tI", "tD", "Ts", "Ro", "To", "b", "r-ref", "y", "kp"/*, "x"*/};
const char  parc[]  = {
  'S', 'P', 'I', 'D', 'T', '0', '1', 'b', 'r', 'y', 'k'/*, 'x' */};
float* vara[]  = {
  &pidSP, &Pb, &Ti, &Td, &Ts, &R0, &T0, &beta, &Rref, &u, &kp/*, &pidPV */};
#define PARNUM sizeof(parc)
#define PARSAVE 9 /* first six save to eeprom */

void calck(void)
{
  if (Ts<=0) Ts=0.250;
  tinterval = Ts*1000.0;
  if (Pb!=0) kp = 255.0/Pb; 
  else kp = 0;
  if (Ti!=0) ki = Ts/Ti; 
  else ki = 0;
  kd = Td/Ts;
}

void setup()
{
  mySerial.begin(9600);
  ltime = millis()-tinterval;
  sprint(":INI\r\n"); 
  if (!lpar())
    sprint(":eeprom?\r\n"); 
}

void procvalue(int a)
{
  pidPV = a;
  if (Rref!=0) pidPV = Rref/(1024.0/(pidPV+.8)-1.0);
  // ln(r/r0)=beta*(1/T-1/T0)  
  if (R0!=0) pidPV = 1.0/(log(pidPV/R0)/beta+1.0/T0)-273.15;
}

void tcloop()
{

  e = 0.8*e + 0.2*(pidPV-pidSP);

  u = u - kp*((e-e1) + ki*e + kd*(e-2.0*e1+e2));

  if (u<0) u=0; 
  else if(u>255)u=255;

  e2=e1;  
  e1=e;
}

void ppar(int i) 
{
  sprint(":");
  sprint(parn[i]);
  fprint(*(vara[i]));
  sprint("\r\n");
}

void gpar()
{
  byte i;
  float x2;
  char pt=mySerial.read();

  if(pt=='\n'||pt=='\r'||pt==' ') return;

  if(pt=='?') {
    for(i=0; i<PARNUM; i++) ppar(i);
    return; 
  }

  x2 = mySerial.parseFloat();

  if(pt=='W' && x2==1) { 
    wpar();   
    sprint(":WR\r\n"); 
    return; 
  }

  for(i=0; i<PARNUM; i++) if (pt==parc[i]) break;
  if(i<PARNUM) 
  {
    *(vara[i]) = x2;
    if(i<PARSAVE) calck();
    ppar(i);
  } 
  else
    sprint(":?\r\n"); 
}

/* eeprom structure:
 0-1 CRC
 2   index
 3-6 float
 7   index FF end
 */

#define EEPROM_MAX 126
void crcEEprom()
{
  byte x;
  crc = CRC_START;
  for(int i=2; i<EEPROM_MAX && (x=eerd(i++))!=0xFF;) {
    calculateCRC(x);
    for(int y = 0; y < 4; y++)
      calculateCRC(eerd(i++));
  }
}

void wpar()
{
  int i;
  for(int j=0, i=2; i < EEPROM_MAX && j < PARSAVE; j++) {
    eewr(i++, parc[j]);
    byte* p = (byte*)(void*)vara[j];
    for (byte y = 0; y < 4; y++)
      eewr(i++, *p++);
  }
  eewr(i, 0xff);
  crcEEprom();
  eewr(0, lowByte(crc));
  eewr(1, highByte(crc));
}

int lpar()
{
  int i,j;
  byte x;

  crcEEprom();
  if (crc != word(eerd(1), eerd(0))) return 0;

  for(i=2; i < EEPROM_MAX && (x=eerd(i++)) != 0xFF;i+=4) {
    for(j=0; j<PARNUM; j++) 
      if (x==parc[j]) {
        byte* p = (byte*)(void*)vara[j];
        for (byte y = 0; y < 4; y++)
          *p++ = eerd(i+y);
      }
  }
  calck();
  return 1;
}

void loop() // run over and over
{
  unsigned long t;

  if (mySerial.available())
    gpar();

  t = millis();
  if (t - ltime >= tinterval) {
    ltime = t;
    procvalue(analogRead(A2));
    tcloop();
    analogWrite(1, u);
    fprint(pidPV);
    sprint(" ");
    fprint(u);
    sprint("\r\n");
  }
}

/* CRC16 Definitions */

static const uint16_t crc_table[16] = {
  0x0000, 0xcc01, 0xd801, 0x1400, 0xf001, 0x3c00, 0x2800, 0xe401,
  0xa001, 0x6c00, 0x7800, 0xb401, 0x5000, 0x9c01, 0x8801, 0x4400
};

uint16_t calculateCRC(byte x) 
{ 
  crc ^= x;
  crc = (crc >> 4) ^ crc_table[crc & 0x0f];
  crc = (crc >> 4) ^ crc_table[crc & 0x0f];
  return crc; 
}






