
#include <EEPROM.h>

enum { pulser_pin=2, sensor_pin=3, indicator=13 };
enum { Sinit, Sidle, Sdetect, Sweld, Swait };
char *xlt[]={ "Init", "Idle", "Detect", "Weld", "Wait" };
  
int state;
uint32_t t0;
uint32_t now;
uint32_t mt1;
int f1,f1a;
uint32_t wt;
uint32_t duty;
uint32_t num;

#define EEBASE 2
#define EESTRUCT 0xE3
bool eeload();
void eesave();

#define CRC_START 0xFFFF
uint16_t crc;

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

/* main enties */

void setup()
{
  digitalWrite(pulser_pin, 1);
  pinMode(pulser_pin, OUTPUT);
  pinMode(sensor_pin, INPUT);
  pinMode(indicator, OUTPUT);
  Serial.begin(115200);
  state=Sinit;
  t0 = 0;
  mt1 = 0;
  f1 = 0; f1a=1024; // 1/alpha=2^10
  wt = 100; // 0.1ms
  duty=50; // 50%
  num=1;  // single pulse
  if(!eeload()) Serial.println("eeprom empty");
}

void loop()
{
  now = millis();
  if (now-t0 >= 1) t0=now, fsm();
  if (Serial.available()) serialdata();
}

/*  routines */

bool delayus(uint32_t d)
{
  uint32_t t=micros();
  while(micros()-t < d);
  return 1;
}

void pulse() {
  uint32_t n=num;
  uint32_t d=wt*100/duty-wt;

  do {
    pinlit();
    digitalWrite(pulser_pin, 0);
    delayus(wt);
    digitalWrite(pulser_pin, 1);
    pindark();
    if (--n) delayus(d);
    else break;
  } while(1);
}

bool delay_mt1(uint32_t ms)
{
  if (now-mt1 > ms) { mt1 = now; return 0; }
  return 1;
}

int filter() {
  int x = digitalRead(sensor_pin);
  if (x && f1 < f1a) f1++;
  if (!x && f1 > 0) f1--;
  if (f1 >= f1a>>1) return 1;
  else return 0;
}

void toState(int s) {
  state = s;
  delay_mt1(0);
  Serial.println(xlt[state]);
}

void fsm()
{
  switch(state) {
  default: state=Sinit;
  case Sinit: 
    digitalWrite(pulser_pin, 1);
    filter();
    if (delay_mt1(250)) break;
    if (filter() == 0) toState(Sidle);
    break;
  case Sidle:
    if (filter() == 1) { toState(Sdetect); pindark(); }
    else if (!delay_mt1(100)) pinflash();
    break;
  case Sdetect:
    filter();
    if (delay_mt1(500)) break;
    if (filter()==1) toState(Sweld); else toState(Sidle);
    break;
  case Sweld:
    pulse();
    toState(Swait);
    break;
  case Swait:
    filter();
    if (delay_mt1(200)) break;
    if (filter()==0) toState(Sidle);
    break;
  }
}

   
void serialdata()
{
  switch(Serial.read()) {
    case 't': wt=(uint32_t)Serial.parseInt();  eesave(); goto dumpit;
    case 'd': duty=(uint32_t)Serial.parseInt();  eesave(); goto dumpit;
    case 'n': num=(uint32_t)Serial.parseInt();  eesave();
    dumpit:
    case '?': Serial.print("weld time (us):"); Serial.println(wt); 
              Serial.print("duty cycle (%):"); Serial.println(duty); 
              Serial.print("pulse count:"); Serial.println(num); 
              Serial.print("f1:"); Serial.println(f1);
              Serial.print("sense:"); Serial.println(digitalRead(sensor_pin));
              Serial.print("state:"); Serial.println(xlt[state]);
              break;
  }
}

void pinflash()
{
  digitalWrite(indicator, 1-digitalRead(indicator));
}

void pinlit()
{
  digitalWrite(indicator, 1);
}

void pindark()
{
  digitalWrite(indicator, 0);
}

bool eeload()
{
  byte x[16];
  crc=CRC_START;
  for(int i=2; i < sizeof(x); i++)
    calculateCRC(x[i]=EEPROM.read(EEBASE+i));
  if (crc != word(EEPROM.read(EEBASE+1),EEPROM.read(EEBASE)) || x[2] != EESTRUCT) return 0;
  wt=*(uint32_t*)(x+4);
  duty=*(uint32_t*)(x+8);
  num=*(uint32_t*)(x+12);
  return 1;
}

void eesave()
{
    byte x[16];
    x[2]=EESTRUCT;
    *(uint32_t*)(x+4) = wt;
    *(uint32_t*)(x+8) = duty;
    *(uint32_t*)(x+12) = num;
    crc=CRC_START;
    for(int i=2; i < sizeof(x); i++)
      calculateCRC(x[i]);
    x[0]=lowByte(crc);
    x[1]=highByte(crc);
    for(int i=0; i < sizeof(x); i++)
      EEPROM.write(EEBASE+i,x[i]);

}


