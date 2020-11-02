
/*
  uses an nRF24L01p connected to an Arduino
  Connects are:  SS=D10  MOSI=D11  MISO=D12  SCK=D13  CE=D9
   front view        edge view
                   GND CE CK MISO
                    1  3  5  7
   2  4  6  8       ----------
   1  3  5  7       2  4  6  8 INT
                   VCC CS MOSI
*/

#include <SPI.h>
#include "interface.h"
#define DEBUG

#define EEPROM_ID_OFFSET    10    // Module ID (4 bytes)

uint32_t MProtocol_id_master;

static void random_init(void);
static uint32_t random_value(void);
static uint32_t random_id(uint16_t address, uint8_t create_new);


struct BayangData myData;

void readpot()
{ // 450 uSec
  myData.roll = analogRead(A0);
  myData.pitch = analogRead(A1);
  myData.throttle = analogRead(A2);
  myData.yaw = analogRead(A3);
  myData.aux1=0;
  myData.flags2=0;
  myData.flags3=0;
}

int sa=0, ca=511, sb=511, cb=0;
int  limadd(int a, int b) 
{
  a+=b;
  if(a >  511) a=511;
  if(a < -511) a=-511;
  return a;
}

void readtest()
{
    myData.roll=511+sa;
    myData.pitch=511+cb;
    myData.throttle=511+sb;
    myData.yaw=511+ca;
    myData.aux1=0;
    myData.flags2=0;
    myData.flags3=0;

    int ts=sa/128,  tc=ca/128;
    sa=limadd(sa,tc), ca=limadd(ca,-ts);
    ts=sb/32,  tc=cb/32;
    sb=limadd(sb,tc), cb=limadd(cb,-ts);
}

void serialdump() {
  Serial.print(myData.roll); Serial.print(' ');
  Serial.print(myData.pitch); Serial.print(' ');
  Serial.print(myData.throttle); Serial.print(' ');
  Serial.println(myData.yaw);
}

uint32_t ref_t;
uint32_t timout;

void setup()
{
  Serial.begin(250000);

  // true random with entropy
  random_init();
  randomSeed(random_value());
  
  MProtocol_id_master=random_id(EEPROM_ID_OFFSET,false);
  Serial.print("MProtocol_id_master="); Serial.println(MProtocol_id_master,HEX);

  set_rx_tx_addr(MProtocol_id_master);
  timout=BAYANG_TX_init();
  ref_t = micros();
  BAYANG_TX_data(&myData);
  BAYANG_TX_bind(); // autobind
  Serial.println("sa ca sb cb");
}

void loop()
{
  uint32_t t=micros();

  if (t-ref_t >= timout) {
    ref_t = t;
  //readpot(); 
    readtest();
    timout= BAYANG_TX_callback();
    //serialdump();
  }

}

#define EE_ADDR uint8_t*

static uint32_t random_id(uint16_t address, uint8_t create_new)
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


static void random_init(void)
{
  cli(); 
  MCUSR = 0; 
  WDTCSR |= _BV(WDCE);
  WDTCSR = _BV(WDIE);
  sei();
}

static uint32_t random_value(void)
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
