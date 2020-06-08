#include "geiger.h"
#include <EEPROM.h>

/* eeprom structure:
 0-1 CRC
 2   index
 3-6 data(size=4)
 7   index FF end
 */


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

#define CRC_START 0xFFFF
extern uint16_t crc;
extern uint16_t calculateCRC16(byte x);

char  parc[] = { 'u', 'f', 'F', 'c'};
void* parv[] = { (void*)&setp, (void*)((int*)&cpmfactor), (void*)(1+(int*)&cpmfactor), (void*)&use_rcpm };

int lpar()
{
  int i,j;
  byte x;

  i=crcEEprom();
  if (crc != word(eerd(i), eerd(i+1))) return 0; // word(h,l)

  for(i=EEPROM_START; i<EEPROM_MAX; i+=PARSIZE) {
    x=eerd(i++);
    if(x==0xFF) break;
    for(j=0; j<PARNUM; j++) 
      if (x==parc[j]) {
        byte* p = (byte*)parv[j];
        for (byte y = 0; y < PARSIZE; y++) *p++ = eerd(i+y);
      }
  }
  return 1;
}

void wpar()
{
  int i,j;
  for(j=0, i=EEPROM_START; i < EEPROM_MAX && j < PARSAVE; j++) {
    eewr(i++, parc[j]);
    byte* p = (byte*)(parv[j]);
    for (byte y = 0; y < PARSIZE; y++)
      eewr(i++, *p++);
  }
  eewr(i, 0xff);
  i=crcEEprom();
  eewr(i++, highByte(crc));
  eewr(i++, lowByte(crc));
}

void clrpar()
{
  eewr(EEPROM_START, 0xff);
  crcEEprom();
  eewr(EEPROM_START+1, highByte(crc));
  eewr(EEPROM_START+2, lowByte(crc));
}


void par_show()
{
#ifdef USE_SERIAL
  byte x, *p;
  uint16_t pd[2];
  int i;

  for(crc = CRC_START, i=EEPROM_START; i<EEPROM_MAX;) {
    x=eerd(i++);
    calculateCRC16(x);
    if (x<0x20 || x>0x7E) {MYSERIAL.print('x'); MYSERIAL.print(x, HEX);} else MYSERIAL.print((char)x);
    MYSERIAL.print(' ');
    if (x==0xFF) break;
    p=(byte*)pd;
    for(int y = 0; y < PARSIZE; y++)
      calculateCRC16(*p++=eerd(i++));
    MYSERIAL.println(pd[0]);
  }
  MYSERIAL.print("CRC"); MYSERIAL.print(crc,HEX);
  crc=word(eerd(i), eerd(i+1));
  MYSERIAL.print(" read"); MYSERIAL.println(crc,HEX);
#endif
}

int crcEEprom()
{
  byte x;
  int i;

  for(crc = CRC_START, i=EEPROM_START; i<EEPROM_MAX;) {
    x=eerd(i++);
    calculateCRC16(x);
    if (x==0xFF) break;
    for(int y = 0; y < PARSIZE; y++)
      calculateCRC16(eerd(i++));
  }
  return i;
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






