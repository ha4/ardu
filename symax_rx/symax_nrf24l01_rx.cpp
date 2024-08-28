#include <Arduino.h>
#include "symax_nrf24l01_rx.h"
#include "nrf24l01.h"

#define DEBUG

#ifdef DEBUG
#define DEBUGLN(x) Serial.print(x); Serial.print('@'); prhex(rf_chan)
#else
#define DEBUGLN(x)
#endif

enum {
  SYMAX_INIT = 0,
  SYMAX_NO_BIND,
  SYMAX_WAIT_FSYNC0,
  SYMAX_WAIT_FSYNC,
  SYMAX_BOUND,
  SYMAX_LOST_BOUND
};

static byte phase = SYMAX_INIT;

#define BIND_COUNT          346
#define FIRST_PACKET_DELAY  12000
#define PACKET_PERIOD        4000
#define INITIAL_WAIT          500

#define PAYLOADSIZE 10
#define MAX_PACKET_SIZE 15

static struct SymaXData *proto_data=NULL;
static byte packet[MAX_PACKET_SIZE];
static word counter;
static uint32_t packet_counter;
static byte tx_power;
static byte throttle, rudder, elevator, aileron, flags;
static byte rx_tx_addr[5];
static byte update;

// frequency channel management
#define MAX_RF_CHANNELS    4
volatile byte current_chan;
volatile static byte rf_chan;
static byte chans[MAX_RF_CHANNELS];

static byte checksum(byte *data)
{
    byte sum = data[0];

    for (int i=1; i < PAYLOADSIZE-1; i++)
            sum ^= data[i];
    
    return sum + 0x55;
}

#ifdef DEBUG
void prhex(byte a)
{
      if (a < 16) Serial.print('0');
      Serial.print(a,HEX);
}
#endif

/*
  Access level routines
 */

// Generate address to use from TX id and manufacturer id (STM32 unique id)
byte initialize_rx_tx_addr()
{
      if (packet[9] != checksum(packet)) return 0;
      if (packet[0] != 0xa2 || packet[5] != 0xaa || packet[6] != 0xaa || packet[7] != 0xaa || packet[8] != 0x00)
          return 0;

      rx_tx_addr[4] = packet[0];
      rx_tx_addr[3] = packet[1];
      rx_tx_addr[2] = packet[2];
      rx_tx_addr[1] = packet[3];
      rx_tx_addr[0] = packet[4];

      NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, 5);
      
      return 1;
}


int rxFlag()
{
    return (NRF24L01_ReadReg(NRF24L01_07_STATUS) & (1 << NRF24L01_07_RX_DR)) != 0;
}

void freqHop()
{
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_chan=chans[current_chan]);
    NRF24L01_FlushRx();
    current_chan = (current_chan + 1) % 4;
}

#define TX_EMPTY    4
#define RX_FULL     1
#define RX_EMPTY    0

void rx_ini()
{
        NRF24L01_Initialize();

        NRF24L01_WriteReg(NRF24L01_00_CONFIG, _BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO)); 
        delayMicroseconds(150);
        NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknoledgement
        NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // 0x3F Enable all data pipes (even though not used?)
        NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, 8);

        NRF24L01_SetBitrate(NRF24L01_BR_250K);

        NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
        NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, PAYLOADSIZE);     // payload width
        NRF24L01_WriteReg(NRF24L01_17_FIFO_STATUS, 0x00); // Just in case, no real bits to write here

        NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, (byte*)"\xab\xac\xad\xae\xaf", 5);

        delayMicroseconds(50);
        NRF24L01_ReadReg(NRF24L01_07_STATUS);
        
        memcpy(chans, (byte*)"\x4b\x30\x40\x20", 4); // chan list first data
        current_chan = 0;
        packet_counter = 0;
        freqHop();

        NRF24L01_SetTxRxMode(RX_EN);
}

void receive_packet()
{
      NRF24L01_WriteReg(NRF24L01_07_STATUS, _BV(NRF24L01_07_RX_DR));

      while ((NRF24L01_ReadReg(NRF24L01_17_FIFO_STATUS) & _BV(RX_EMPTY)) == 0) {
              NRF24L01_ReadPayload(packet, PAYLOADSIZE);
      }

      NRF24L01_FlushRx();
      packet_counter++;
}


/*
   Protocol level routines
 */

// channels determined by last byte (LSB) of tx address
// set_channels(rx_tx_addr[0] & 0x1f) - little endian
static void set_channels(byte laddress)
{ 
  static const byte start_chans_1[] = {0x0a, 0x1a, 0x2a, 0x3a};
  static const byte start_chans_2[] = {0x2a, 0x0a, 0x42, 0x22};
  static const byte start_chans_3[] = {0x1a, 0x3a, 0x12, 0x32};

  uint32_t *pchans = (uint32_t *)chans;   // avoid compiler warning

  if (laddress < 0x10) {
    if (laddress == 6) laddress = 7;
    for(byte i=0; i < 4; i++) chans[i] = start_chans_1[i] + laddress;
  } else if (laddress < 0x18) {
    for(byte i=0; i < 4; i++)  chans[i] = start_chans_2[i] + (laddress & 0x07);
    if (laddress == 0x16) { chans[0]++; chans[1] ++; }
  } else if (laddress < 0x1e) {
    for(byte i=0; i < 4; i++) chans[i] = start_chans_3[i] + (laddress & 0x07);
  } else if (laddress == 0x1e) {
      *pchans = 0x38184121;
  } else {
      *pchans = 0x39194121;
  }
}

/* packet: T[7:0] E[7:0] R[7:0] A[7:0] fVid,fPict[7:6] fLow[7],tE[5:0] fFlip[6],tR[5:0] fHLess[7],tA[5:0]*/
void decode_packet(struct SymaXData *val)
{
	if (packet[9] != checksum(packet) || packet[8] != 0x00) return;
	update = 1;

	val->throttle=packet[0];
	val->elevator=CHAN_SYMAX(packet[1]);
	val->rudder=CHAN_SYMAX(packet[2]);
	val->aileron=CHAN_SYMAX(packet[3]);
	val->trims[0]=TRIM_SYMAX(packet[5]);
	val->trims[1]=TRIM_SYMAX(packet[6]);
	val->trims[2]=TRIM_SYMAX(packet[7]);
	val->flags4=packet[4];
	val->flags5=packet[5];
	val->flags6=packet[6];
	val->flags7=packet[7];
}

uint16_t symax_rx_run()
{
  switch(phase) {
  case SYMAX_INIT:
        rx_ini();
        phase = (SYMAX_NO_BIND);
        return INITIAL_WAIT;

  case SYMAX_NO_BIND:
        if (!rxFlag()) {  // switch search channels
            freqHop();
            return PACKET_PERIOD*7;
        }

        receive_packet();

        if(initialize_rx_tx_addr()) { // binding packet
          DEBUGLN('>');
          set_channels(rx_tx_addr[0] & 0x1f);
          current_chan = 0;
          packet_counter = 0;
          counter = 0;
          freqHop();
          phase = (SYMAX_WAIT_FSYNC);
          return INITIAL_WAIT;
        } else {
                DEBUGLN('?');
        }

        return PACKET_PERIOD;
  
  case SYMAX_WAIT_FSYNC:
        if (rxFlag()) {
          receive_packet(); // sync packet throttle should be 0
          DEBUGLN('}');
          freqHop();
          phase = (SYMAX_BOUND);
          counter = 0;
          return PACKET_PERIOD*2;
        }
        if (counter++ > 40) phase = (SYMAX_INIT);
        return PACKET_PERIOD * 8; // repeat 4 channels twice

  case SYMAX_LOST_BOUND:
        if (!rxFlag()) {
              freqHop();
              // if (counter++ > 40) phase = (SYMAX_INIT); do not unbound
              return PACKET_PERIOD * 8; // repeat 4 channels twice
        }
        phase = (SYMAX_BOUND);

  case SYMAX_BOUND:
        if (!rxFlag()) {  // switch search channels
            DEBUGLN('R');
            //freqHop();
            if (counter++ > 40) {
              DEBUGLN('L');
              counter = 0;
              phase = (SYMAX_LOST_BOUND);
            }
            return PACKET_PERIOD/16;
        } 
        counter = 0;
        receive_packet();
        DEBUGLN('.');
	      if (proto_data != NULL) decode_packet(proto_data);
        freqHop();
     
      return PACKET_PERIOD*2;
  }
  return PACKET_PERIOD;
}

void symax_rx_data(struct SymaXData *val)
{
	proto_data=val;
}

int  symax_rx_binding() // 1 binding, -1 no values, 0 - ok data
{
  if (update) {
    update = 0;
    return 0; 
  }
  if (phase==SYMAX_NO_BIND || phase==SYMAX_WAIT_FSYNC) return 1;
  return -1;
}
