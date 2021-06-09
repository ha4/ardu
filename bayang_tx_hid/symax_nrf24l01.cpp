#include <Arduino.h>
#include "symax_nrf24l01.h"
#include "nrf24l01.h"

#define SYMAX_AUTOBIND

#define BIND_COUNT 37
#define FIRST_PACKET_DELAY  12000

#define PACKET_PERIOD        4000
#define INITIAL_WAIT          500

#define PAYLOADSIZE 10
#define MAX_PACKET_SIZE 10

#define MODEL_tx_power TXPOWER_150mW

// frequency channel management
#define MAX_RF_CHANNELS    4

enum {
  SYMAX_INIT = 0,
  SYMAX_BIND,
  SYMAX_DATA
};

static uint8_t rx_tx_addr[5];
static uint8_t packet[MAX_PACKET_SIZE];
static uint16_t bind_counter;
static uint32_t packet_counter;
static uint8_t symax_tx_power;
static uint8_t current_chan_no;
static uint8_t chans[MAX_RF_CHANNELS];
static uint8_t rf_chan;
static uint8_t symax_phase;
static struct SymaXData *symax_proto_data = NULL;

void prhex(byte a)
{
      if (a < 16) Serial.print('0');
      Serial.print(a,HEX);
}

// RANDOM GEEENERATOR
uint32_t symax_rand32_r(uint32_t *lfsr, uint8_t b)
{
  static const uint32_t LFSR_FEEDBACK = 0x80200003ul;
  static const uint32_t LFSR_INTAP = 32 - 1;
  for (int i = 0; i < 8; ++i) {
    *lfsr = (*lfsr >> 1) ^ ((-(*lfsr & 1u) & LFSR_FEEDBACK) ^ ~((uint32_t)(b & 1) << LFSR_INTAP));
    b >>= 1;
  }
  return *lfsr;
}

// Generate address to use from TX id and manufacturer id (STM32 unique id)
void symax_tx_id(uint32_t id)
{
  
  uint32_t lfsr = id; // default 0x7F7FC0D7ul;

  for (uint8_t i = 0; i < 4; i++) {
    rx_tx_addr[i] = lfsr & 0xff;
    symax_rand32_r(&lfsr, i);
  }
  rx_tx_addr[4] = 0xa2;
}

static void set_channels()
{
  static const uint8_t start_chans_1[] = {0x0a, 0x1a, 0x2a, 0x3a};
  static const uint8_t start_chans_2[] = {0x2a, 0x0a, 0x42, 0x22};
  static const uint8_t start_chans_3[] = {0x1a, 0x3a, 0x12, 0x32};

  uint32_t *pchans = (uint32_t *)chans;   // avoid compiler warning
  // channels determined by last byte (LSB) of tx address
  // set_channels(rx_tx_addr[0] & 0x1f) - little endian
  uint8_t laddress = rx_tx_addr[0] & 0x1f;

  if (laddress < 0x10) {
    if (laddress == 6) laddress = 7;
    for (uint8_t i = 0; i < 4; i++) chans[i] = start_chans_1[i] + laddress;
  } else if (laddress < 0x18) {
    for (uint8_t i = 0; i < 4; i++)  chans[i] = start_chans_2[i] + (laddress & 0x07);
    if (laddress == 0x16) {
      chans[0]++;
      chans[1] ++;
    }
  } else if (laddress < 0x1e) {
    for (uint8_t i = 0; i < 4; i++) chans[i] = start_chans_3[i] + (laddress & 0x07);
  } else if (laddress == 0x1e) {
    *pchans = 0x38184121;
  } else {
    *pchans = 0x39194121;
  }
}

static uint8_t symax_checksum(uint8_t *data)
{
  uint8_t sum = data[0];

  for (int i = 1; i < PAYLOADSIZE - 1; i++)
    sum ^= data[i];

  return sum + 0x55;
}

static void symax_build_channels(struct SymaXData *val)
{
  packet[0] = val->throttle;
  packet[1] = SYMAX_CHAN(val->elevator);
  packet[2] = SYMAX_CHAN(val->rudder);
  packet[3] = SYMAX_CHAN(val->aileron);
  packet[4] = val->flags4;
  packet[5] = val->flags5 | FLAG5_SET1 | SYMAX_TRIM(val->trims[0]); // elevator trim
  packet[6] = val->flags6 | SYMAX_TRIM(val->trims[1]); // rudder trim
  packet[7] = val->flags7 | SYMAX_TRIM(val->trims[2]); // aileron trim
}

static void symax_send_packet(uint8_t bind)
{
  static const uint8_t bchans[] = {0x4b,0x30,0x40,0x20};
  if (bind) {
    packet[0] = rx_tx_addr[4];
    packet[1] = rx_tx_addr[3];
    packet[2] = rx_tx_addr[2];
    packet[3] = rx_tx_addr[1];
    packet[4] = rx_tx_addr[0];
    packet[5] = 0xaa;
    packet[6] = 0xaa;
    packet[7] = 0xaa;
  } else {
    if (symax_proto_data != NULL) symax_build_channels(symax_proto_data);
    else memset(packet, 0, 8);
  }
  packet[8] = 0x00;
  packet[9] = symax_checksum(packet);

  rf_chan = bind?bchans[current_chan_no]:chans[current_chan_no];
  // clear packet status bits and TX FIFO
  NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
  NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x2e);
  NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_chan);
  NRF24L01_FlushTx();

  NRF24L01_WritePayload(packet, PAYLOADSIZE);

  // use each channel twice
  if (packet_counter++ % 2) current_chan_no = (current_chan_no + 1) & 3;

  // Check and adjust transmission power. We do this after transmission to not bother
  // with timeout after power settings change -  we have plenty of time until next packet.
  if (symax_tx_power != MODEL_tx_power)
    NRF24L01_SetPower(symax_tx_power = MODEL_tx_power);
//    Serial.print('@'); prhex(rf_chan);
}

static void symax_init_nrf()
{
  symax_tx_power = MODEL_tx_power;

  NRF24L01_Initialize();

  NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknoledgement
  NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x00);  // 0x3F Enable all data pipes (even though not used?)
  NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address
  NRF24L01_WriteReg(NRF24L01_05_RF_CH, 8);

  NRF24L01_SetBitrate(NRF24L01_BR_250K);
  NRF24L01_SetPower(symax_tx_power);

  NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
  NRF24L01_WriteReg(NRF24L01_17_FIFO_STATUS, 0x00); // Just in case, no real bits to write here

  NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, (uint8_t*)"\xab\xac\xad\xae\xaf", 5); // binding address

  NRF24L01_ReadReg(NRF24L01_07_STATUS);
  NRF24L01_SetTxRxMode(TX_EN);
}

void symax_init()
{
    symax_init_nrf();

#ifdef SYMAX_AUTOBIND
    symax_phase = SYMAX_INIT;
#else
    symax_phase = SYMAX_BIND;
    bind_counter = 1;
#endif
    current_chan_no = 0;
    packet_counter = 0;
}

void symax_bind()
{
    symax_init_nrf();
    symax_phase = SYMAX_INIT;
}

uint16_t symax_callback()
{
  switch (symax_phase) {
    case SYMAX_INIT:
      bind_counter = BIND_COUNT;
      symax_phase = SYMAX_BIND;
        NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, (uint8_t*)"\xab\xac\xad\xae\xaf", 5); // binding address

      return FIRST_PACKET_DELAY;

    case SYMAX_BIND:
      if (--bind_counter == 0) {
        set_channels();
        NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5); // sync packet first, throttle should be 0x00
        current_chan_no = 0;
        packet_counter = 0;
        symax_phase = SYMAX_DATA;
      } else
        symax_send_packet(1);
      break;

    case SYMAX_DATA:
      symax_send_packet(0);
      break;
  }
  return PACKET_PERIOD;
}

int8_t symax_state()
{
  return (symax_phase == SYMAX_DATA);
}

void symax_data(struct SymaXData *x)
{
  symax_proto_data = x;
}
