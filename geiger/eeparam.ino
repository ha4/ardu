#include <EEPROM.h>

/* 
 * Enable EEPROM save over reprograming flash (tiny85):
 *
 * C:\Program Files (x86)\Arduino\hardware/tools/avr/bin/avrdude 
 * -CC:\Program Files (x86)\Arduino\hardware/tools/avr/etc/avrdude.conf
 * -v -v -v -v -pattiny85 -cusbtiny -Pusb -e 
 * -Uefuse:w:0xff:m -Uhfuse:w:0xd7:m -Ulfuse:w:0xe2:m 
 */

#define eerd(a) EEPROM.read(a)
#define eewr(a, d) EEPROM.write(a,d)

void crcEEprom()
{
  byte x;
  crc = CRC_START;
  for(int i=2; i<EEPROM_MAX && (x=eerd(i++))!=0xFF;) {
    calculateCRC16(x);
    for(int y = 0; y < 4; y++)
      calculateCRC16(eerd(i++));
  }
}

void wpar()
{
  int i;
  for(int j=0, i=2; i < EEPROM_MAX && j < PARSAVE; j++) {
    eewr(i++, parc[j]);
    byte* p = (byte*)(void*)parv[j];
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
        byte* p = (byte*)(void*)parv[j];
        for (byte y = 0; y < 4; y++)
          *p++ = eerd(i+y);
      }
  }
  return 1;
}

void clrpar()
{
  eewr(2, 0xff);
  crcEEprom();
  eewr(0, lowByte(crc));
  eewr(1, highByte(crc));
}

/* CRC16 Definitions */

uint16_t crc;

static const uint16_t crc_table[16] = {
  0x0000, 0xcc01, 0xd801, 0x1400, 0xf001, 0x3c00, 0x2800, 0xe401,
  0xa001, 0x6c00, 0x7800, 0xb401, 0x5000, 0x9c01, 0x8801, 0x4400
};

uint16_t calculateCRC16(byte x) 
{ 
  crc ^= x;
  crc = (crc >> 4) ^ crc_table[crc & 0x0f];
  crc = (crc >> 4) ^ crc_table[crc & 0x0f];
  return crc; 
}






