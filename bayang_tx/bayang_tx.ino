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

#define EEPROM_ID_OFFSET    10    // Module ID (4 bytes)

uint32_t MProtocol_id_master;

static void random_init(void);
static uint32_t random_value(void);
static uint32_t random_id(uint16_t address, uint8_t create_new);


#define CHAN_NUM 6
uint16_t channels[CHAN_NUM];

void readpot()
{ // 450 uSec
  channels[0] = analogRead(A0);
  channels[1] = analogRead(A1);
  channels[2] = analogRead(A2);
  channels[3] = analogRead(A3);
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
    channels[0]=1500+sa;
    channels[1]=1500+cb;
    channels[2]=1500+sb;
    channels[3]=1500+ca;
    int ts=sa/128,  tc=ca/128;
    sa=limadd(sa,tc), ca=limadd(ca,-ts);
    ts=sb/32,  tc=cb/32;
    sb=limadd(sb,tc), cb=limadd(cb,-ts);
}

void serialdump() {
  for (int i = 0; i < CHAN_NUM; i++) {
    Serial.print(channels[i]);
    Serial.print(' ');
  }
  Serial.println();
}

uint32_t ref_t;
uint32_t timout;

void setup()
{
  Serial.begin(115200);
  random_init();
  randomSeed(random_value());
  MProtocol_id_master=random_id(EEPROM_ID_OFFSET,false);
  set_rx_tx_addr(MProtocol_id_master);
  timout=initBAYANG();
  ref_t = micros();
//  Serial.print("RandomID:");  Serial.println(MProtocol_id_master);
  Serial.println("sa ca sb cb");
  
}

void loop()
{
  uint32_t t=micros();

  //readpot(); 
  readtest();
  if (t-ref_t >= timout) {
    ref_t = t;
    timout= BAYANG_callback(channels);
  }
  //serialdump();
  delay(1);
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
  cli();          // Temporarily turn off interrupts, until WDT configured
  MCUSR = 0;        // Use the MCU status register to reset flags for WDR, BOR, EXTR, and POWR
  WDTCSR |= _BV(WDCE);  // WDT control register, This sets the Watchdog Change Enable (WDCE) flag, which is  needed to set the prescaler
  WDTCSR = _BV(WDIE);   // Watchdog interrupt enable (WDIE)
  sei();          // Turn interupts on
}

static uint32_t random_value(void)
{
  while (!gWDT_entropy);
  return gWDT_entropy;
}

// Random interrupt service routine called every time the WDT interrupt is triggered.
// It is only enabled at startup to generate a seed.
ISR(WDT_vect)
{
  static uint8_t gWDT_buffer_position = 0;
#define gWDT_buffer_SIZE 32
  static uint8_t gWDT_buffer[gWDT_buffer_SIZE];
  gWDT_buffer[gWDT_buffer_position] = TCNT1L; // Record the Timer 1 low byte (only one needed)
  gWDT_buffer_position++;                     // every time the WDT interrupt is triggered
  if (gWDT_buffer_position >= gWDT_buffer_SIZE)
  {
    // The following code is an implementation of Jenkin's one at a time hash
    for (uint8_t gWDT_loop_counter = 0; gWDT_loop_counter < gWDT_buffer_SIZE; ++gWDT_loop_counter)
    {
      gWDT_entropy += gWDT_buffer[gWDT_loop_counter];
      gWDT_entropy += (gWDT_entropy << 10);
      gWDT_entropy ^= (gWDT_entropy >> 6);
    }
    gWDT_entropy += (gWDT_entropy << 3);
    gWDT_entropy ^= (gWDT_entropy >> 11);
    gWDT_entropy += (gWDT_entropy << 15);
    WDTCSR = 0; // Disable Watchdog interrupt
  }
}
