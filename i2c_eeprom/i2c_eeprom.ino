
#include <Wire.h>

uint8_t ee_adr = 0x50;
uint8_t cam_adr = 0x33;
uint16_t ee_size = 2048;
uint16_t ee_bsize = 16;

void setup()
{
  Wire.begin();

  Serial.begin(115200);
  while (!Serial);
  Serial.println("\nI2C eeprom");
}

void print_byte(uint8_t x)
{
  if (x < 0x10)Serial.print('0');
  Serial.print(x, HEX);
  Serial.print(' ');
}

uint16_t scan_hex()
{
  uint16_t w;
  char r;
  for(w=0;;) {
    while(Serial.available()<=0);
    r=Serial.read();
    if (r>='a' && r<='f') r-='a'-'A';
    if (r>='0' && r<='9') {
      w<<=4;
      w+= r-'0';
    } else if (r>='A' && r<='F') {
      w<<=4;
      w+= r-'7';
    } else
      break;
  }
  return w;
}

void put_reg(uint8_t address, uint8_t data)
{
  Wire.beginTransmission(cam_adr);
  Wire.write(address);
  Wire.write(data);
  Wire.endTransmission();
}

uint8_t get_reg(uint8_t address)
{
  Wire.beginTransmission(cam_adr);
  Wire.write(address);
  Wire.endTransmission();
  Wire.requestFrom((int)cam_adr, 1);
  return Wire.read();
}

void write_byte(uint16_t address, uint8_t data)
{
  Wire.beginTransmission(ee_adr + (uint8_t)(address >> 8));
  Wire.write((uint8_t)address & 0xFF);
  Wire.write(data);
  Wire.endTransmission();
}

uint8_t read_byte(uint16_t address)
{
  Wire.beginTransmission(ee_adr + (uint8_t)(address >> 8));
  Wire.write((uint8_t)address & 0xFF);
  Wire.endTransmission();
  Wire.requestFrom(ee_adr + (uint8_t)(address >> 8), 1);
  return Wire.read();
}

void read_blk(uint16_t address, uint16_t sz)
{
  if (sz > 32) sz = 32;
  Serial.print(address, HEX); Serial.print(": ");
  Wire.beginTransmission(ee_adr + (uint8_t)(address >> 8));
  Wire.write((uint8_t)address & 0xFF);
  Wire.endTransmission();

  Wire.requestFrom(ee_adr + (uint8_t)(address >> 8), sz);
  while (Wire.available()) print_byte(Wire.read());
  Serial.println();
}

void read_all()
{
  uint16_t address = 0;
  do {
    read_blk(address, ee_bsize);
    address += ee_bsize;
  } while (address < ee_size);

}

void serial_get()
{
  char b = Serial.read();
  uint16_t a;
  uint8_t d;
  switch (b) {
    default:
      Serial.println(F("???"));
      break;
    case '?': case 'h':
      Serial.println(F("commands: s, b, @, c, g, p, r, w, x|a, ?"));
      Serial.print(F("eeprom size: 0x")); Serial.println(ee_size,HEX);
      Serial.print(F("eeprom block size: 0x")); Serial.println(ee_bsize,HEX);
      Serial.print(F("eeprom address: 0x")); Serial.println(ee_adr, HEX);
      Serial.print(F("cam address: 0x")); Serial.println(cam_adr, HEX);
      break;
    case 's':
      ee_size = scan_hex();
      break;
    case 'b':
      ee_bsize = scan_hex();
      break;
    case '@':
      ee_adr = scan_hex();
      break;
    case 'c':
      cam_adr = scan_hex();
      break;
    case 'x': case 'a':
      read_all();
      break;
    case 'r':
      Serial.println(read_byte(scan_hex()),HEX);
      break;
    case 'g':
      Serial.println(get_reg(scan_hex()),HEX);
      break;
    case 'w':
      a = scan_hex();
      d =scan_hex();
      write_byte(a, d);
      Serial.print(F("Wrote @"));
      goto displ;
    case 'p':
      a = scan_hex();
      d =scan_hex();
      put_reg(a&0xFF, d);
      Serial.print(F("cam wrote @"));
    displ:
      Serial.print(a,HEX);
      Serial.print(' ');
      Serial.println(d,HEX);
      break;
  }
}

void loop()
{
  if (Serial.available() > 0) serial_get();
}
