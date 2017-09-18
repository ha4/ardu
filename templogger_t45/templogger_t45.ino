#include "SerialSend.h"
#include <Arduino.h>
#include "owire.h"

SerialSend mySerial(1); // TX
OneWire  ds(0); // on D0

//OneWire  ds(14); // on D14

uint32_t _mscnt;

//void putch(char c) { Serial.print(c); }
void putch(char c) { mySerial.print(c); }

void setup() {
  mySerial.begin(9600); 
  //Serial.begin(9600); 
  _mscnt = millis();
  delayMicroseconds(30000+1875);
  printn(millis()-_mscnt);
}

void prints(char *s) { while(*s) putch(*s++); }
void printd(uint8_t b) { putch('0' + ((b<10)?b:(b+7))); }
void printh(uint8_t b) { printd(b>>4); printd(b&15); }
void printv(uint8_t *p, uint8_t n) {  for(;n--;p++) printh(*p); }
void printn(int d)
{
  int m;char buf[6]; char *s=buf+5; *s=0;
  do { m=d; d/=10; *--s = '0' + (m-d*10); } while(d);
  prints(s);
}
void printfix(int d, int m)
{
  int f;
  if (d < 0) d=-d, putch('-');
  f = d/m;
  printn(f);
  putch('.');
  f = (d-f*m)*100/m;
  if (f < 10) putch('0');
  printn(f);
}

void loop() // run over and over
{
  byte data[12], addr[8];
  int  tu;
  if (millis() - _mscnt > 1000) { _mscnt=millis(); } else { return; }
  if (!ds.reset()) { prints("NoDevices\r\n"); }
  else {
    for(;;) {
      ds.power(false);
      ds.reset();
      ds.write(0xf0);
      if (!ds.discover(addr)) { ds.start_srch(); break; } 
      if (ds.crc(addr,8)!=0) continue;
      ds.reset();
    //  ds.write(0xCC);     // skip rom
      ds.write(0x55);     // select rom
      ds.write(addr, 8);
      ds.write(0xBE);         // Read Scratchpad
      ds.read(data, 9);
      if (ds.crc(data, 9)!=0) continue;
      tu = data[0] + 256*data[1];
      printv(addr,8); putch(' '); printfix(tu,16); putch(' ');
    }
    ds.reset();
    ds.write(0xCC);     // skip rom
    ds.write(0x44);     // start conversion
    ds.power(true);
    prints("\r\n");
  }
}

