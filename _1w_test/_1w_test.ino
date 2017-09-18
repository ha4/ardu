#include <OneWire.h>

OneWire  ds(15);  // on pin 15, PC1
uint32_t t1(0);

byte timeout(uint32_t *tm, uint32_t now, unsigned int tmout)
{
  if (now - *tm >= tmout) { 
    *tm = now;
    return 1; 
  } 
  else
    return 0;
}

void print_byte(byte h)
{
  if (h < 16) Serial.print("0");
  Serial.print(h, HEX);
}

void setup(void)
{
  pinMode(13, OUTPUT);  
  digitalWrite(13, 1); // PB5 flash
  pinMode(14, OUTPUT);  
  digitalWrite(14, 0); // GND on PC0
  pinMode(16, OUTPUT);  
  digitalWrite(16, 1); // VCC on PC2
  Serial.begin(57600);
}

void loop(void)
{
  uint8_t adr[8];
  int i,a;

  if(!timeout(&t1, millis(), 250)) return;

  ds.start();
  for(a=0;;) {
    if (!ds.reset()) break;
    ds.write(0xF0);
    if(ds.discover(adr)) {
      Serial.print("dev"); 
      for(i = 0; i < 8; i++) print_byte(adr[i]);
      a=(ds.crc8(adr,8)==0);
      Serial.println(a?" OK":" CRC");
    } else {
      Serial.println("fin"); 
      break;
    }
  }
  if (a) digitalWrite(13, 1-digitalRead(13));
}





