#include <Arduino.h>
#include "tx_util.h"

uint32_t MProtocol_id_master;
int8_t txstate;
struct proto_t *txproto;

#define EE_ADDR uint8_t*

/*
 * tx-id management
 */

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

/*
 * protocols list manage
 */

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

void set_protocol(char *name)
{
  int i=0;
  while(tx_lst[i].name!=NULL) i++; // sizeof(tx_lst)/sizeof(struct proto_t);

  txstate=STATE_DOINIT;
  if (name==NULL) { txproto=&tx_lst[0]; return; }
  size_t nl=strlen(name);
  if (nl==0) { txproto=&tx_lst[0]; return; }
  while(i) {
    i--;
    txproto=&tx_lst[i];
    if(strncmp(name,txproto->name,nl)==0) return;
  }
}

char *next_protocol()
{
  static uint8_t i=0;
  // if (i < sizeof(tx_lst)/sizeof(struct proto_t))
  if(tx_lst[i].name != NULL)
    return tx_lst[i++].name;
  i=0;
  return NULL;
}

char *get_protocol()
{
  return txproto->name;
}

/* test circular motion */

#define TEST_LIM 511
#define TEST_SCA 128
#define TEST_SCB 32
#define TEST_CIRC(s,c,fs,fc,sc) fs=s/sc; fc=c/sc; s=limadd(s,fc); c=limadd(c,-fs)

int  limadd(int a, int b) { a+=b;  return max(-TEST_LIM,min(a,TEST_LIM)); }

int read_test(uint8_t chan)
{
    static int sc[] = {0, TEST_LIM, TEST_LIM, 0};
    int ts,tc;
    if (chan==0) {
      TEST_CIRC(sc[0],sc[1],ts,tc,TEST_SCA);
      TEST_CIRC(sc[2],sc[3],ts,tc,TEST_SCB);
    }
    return sc[chan];
}
