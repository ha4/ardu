#include <OneWire.h>

/*
DS18B20 Temperature chip
 */

uint32_t ref_time;
OneWire  bus1(9);  // on pin D15(PC1)
enum { led = 13 }; 

void setup(void) {
//  pinMode(14, OUTPUT);  digitalWrite(14, 0); // GND on D14(PC0)
  Serial.begin(9600);
  pinMode(led, OUTPUT);     
  ref_time = millis();

  bus1.reset();
  bus1.write(0xCC);  // skip ROM
  bus1.write(0x4E); // writescratchpad
  bus1.write(0x7F); // TH=127*C
  bus1.write(0x00); // TL=0*C
  bus1.write(0x7F); // config 0 11 11111 : 12bit
}

void printhex(byte *x, byte sz)
{
   for(;sz--;x++) {
     if (*x<16) Serial.print("0");
     Serial.print(*x,HEX);
   }
}
void loop(void) {
  byte data[12];
  byte i;
  float t;
  int  tu;
  uint32_t tim;

  tim = millis();
  if (tim - ref_time < 1000) return;
  ref_time = tim;

  digitalWrite(led, 1);
  if (!bus1.reset()) { Serial.println("{nobus}"); return; }

  bus1.write(0xCC);  // skip ROM
  bus1.write(0xBE);  // Read Scratchpad
  bus1.vread(data, 9);
 
  if (OneWire::crc8(data, 9)!=0) { Serial.println("{crc}");return; }

  tu = (data[0] + 256.0*data[1]);
  t = tu / 16.0;
  Serial.println(t, 2);

  bus1.reset();
  bus1.write(0xCC);  // skip ROM
  bus1.write(0x44); // start conversion, with parasite power on at the end
  bus1.power(true);
  
  delay(750);
  digitalWrite(led, 0);
}

