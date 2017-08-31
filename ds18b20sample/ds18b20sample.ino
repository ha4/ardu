#include <OneWire.h>

/* DS18B20 Temperature chip i/o
 
 */

OneWire  bus1(15);  // on pin PC1

void setup(void) {
  Serial.begin(9600);
  

}


void print_byte(byte h)
{
    if (h < 16) Serial.print("0");
    Serial.print(h, HEX);
}

void loop(void) {
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];
  float temp;

  // discover cycle  
  if (bus1.reset()) {
     bus1.write(0xf0);
     if ( !bus1.discover(addr)) {
         Serial.print("No more addresses.\n");
         bus1.start();
         return;
     }
  } else {
         Serial.print("No devices.\n");
         return;
  }
  
  Serial.print("R=");
  for( i = 0; i < 8; i++)
    print_byte(addr[i]);

  if ( OneWire::crc8(addr, 7) != addr[7]) {
      Serial.print(" CRC is not valid!\n");
      return;
  } else 
      Serial.print(" CRC ok ");
  
  if ( addr[0] != 0x28) {
      Serial.print("Device is not a DS18B20 family device.\n");
      return;
  } else
      Serial.print("DS18B20 ");

  bus1.reset();
  bus1.write(0x55);  // match ROM
  bus1.vwrite(addr,8);
  bus1.write(0x44); // start conversion, with parasite power on at the end
  bus1.power(true);
  
  delay(750);     // maybe 750ms is enough, maybe not
  
  present = bus1.reset();
  bus1.write(0x55);  // match ROM
  bus1.vwrite(addr,8); 
  bus1.write(0xBE);         // Read Scratchpad
  bus1.vread(data, 9);

  Serial.print("P=");
  Serial.print(present,HEX);
  Serial.print(" ");
  for ( i = 0; i < 9; i++)
    print_byte(data[i]);
  i = OneWire::crc8(data, 8);
  Serial.print(" CRC=");   print_byte(i);

  if (i == data[8]) {
      Serial.print(" temp=");
      temp = data[0];
      temp += 256.0*data[1];
      temp /= 16.0;
      Serial.print(temp);
  }    
  
  Serial.println();
}

