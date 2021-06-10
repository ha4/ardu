#include <Arduino.h>
#include "tx_util.h"

#define EE_ADDR uint8_t*

uint32_t random_id(uint16_t address, uint8_t create_new)
{
  uint32_t id=0;

  if(eeprom_read_byte((EE_ADDR)(address+10))==0xf0 && !create_new)
  {  // TXID exists in EEPROM
    for(uint8_t i=4;i>0;i--)
    {
      id<<=8;
      id|=eeprom_read_byte((EE_ADDR)address+i-1);
    }
    if(id!=0x2AD141A7)  //ID with seed=0
      return id;
  }
  id = random(0xfefefefe) + ((uint32_t)random(0xfefefefe) << 16);

  for(uint8_t i=0;i<4;i++)
    eeprom_write_byte((EE_ADDR)address+i,id >> (i*8));
  eeprom_write_byte((EE_ADDR)(address+10),0xf0);//write bind flag in eeprom.
  return id;
}

volatile uint32_t gWDT_entropy=0;

void random_init(void)
{
  cli(); 
  MCUSR = 0; 
  WDTCSR |= _BV(WDCE);
  WDTCSR = _BV(WDIE);
  sei();
}

uint32_t random_value(void)
{
  while (!gWDT_entropy);
  return gWDT_entropy;
}

ISR(WDT_vect)
{
#define gWDT_NUM 32
  static uint32_t _entropy=0;
  static uint8_t n = 0;

  _entropy += TCNT1L;
  _entropy += (_entropy<<10);
  _entropy ^= (_entropy >> 6);
  if(++n >= gWDT_NUM) {
    _entropy += (_entropy << 3);
    _entropy ^= (_entropy >> 11);
    _entropy += (_entropy << 15);
    gWDT_entropy=_entropy;
    
    WDTCSR = 0; // Disable Watchdog interrupt
  }
}

char* saved_proto(uint16_t address, char *new_name)
{
  uint8_t chk;
  static char n[6];

  chk=0;
  if (new_name==NULL) { // try to get
    for(uint8_t i=0;i<6;i++) chk^=eeprom_read_byte((EE_ADDR)(address+i));
    chk+=0x55;
    if(eeprom_read_byte((EE_ADDR)(address+6))!=0xf6 || eeprom_read_byte((EE_ADDR)(address+7))!=chk)
      return NULL; // fail to read
    for(uint8_t i=0;i<6;i++) n[i]=eeprom_read_byte((EE_ADDR)(address+i));
    return n;
  }
  for(uint8_t i=0;i<6;i++) {
    n[i]=*new_name;
    if (*new_name) new_name++;
    eeprom_write_byte((EE_ADDR)address+i,(uint8_t)n[i]);
    chk^=(uint8_t)n[i];
  }
  chk+=0x55;
  eeprom_write_byte((EE_ADDR)(address+6),0xf6); // write marker
  eeprom_write_byte((EE_ADDR)(address+7),chk); // write chksum

  return new_name;
}
