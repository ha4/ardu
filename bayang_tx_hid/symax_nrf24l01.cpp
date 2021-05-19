#include <Arduino.h>
#include "symax_nrf24l01.h"
#include "nrf24l01.h"


#define BIND_COUNT 346
#define FIRST_PACKET_DELAY  12000

#define PACKET_PERIOD        4000
#define INITIAL_WAIT          500

#define PAYLOADSIZE 10
#define MAX_PACKET_SIZE 10

static uint8_t packet[MAX_PACKET_SIZE];
static uint16_t counter;
static uint32_t packet_counter;
static uint8_t tx_power;
static uint8_t rx_tx_addr[5];
struct SymaXData *symax_proto_data = NULL;

// frequency channel management
#define MAX_RF_CHANNELS    4
static uint8_t current_chan;
static uint8_t chans[MAX_RF_CHANNELS];


#define MODEL_tx_power TXPOWER_150mW

enum {
    SYMAX_INIT=0,
    SYMAX_INIT1,
    SYMAX_BIND2,
    SYMAX_BIND3,
    SYMAX_DATA
};

static uint8_t phase = SYMAX_INIT;


// RANDOM GEEENERATOR
uint32_t rand32_r(uint32_t *lfsr, uint8_t b)
{
    static const uint32_t LFSR_FEEDBACK = 0x80200003ul;
    static const uint32_t LFSR_INTAP = 32-1;
    for (int i = 0; i < 8; ++i) {
        *lfsr = (*lfsr >> 1) ^ ((-(*lfsr & 1u) & LFSR_FEEDBACK) ^ ~((uint32_t)(b & 1) << LFSR_INTAP));
        b >>= 1;
    }
    return *lfsr;
}

// Generate address to use from TX id and manufacturer id (STM32 unique id)
static void initialize_rx_tx_addr()
{
    uint32_t lfsr = 0x7F7FC0D7ul;

    for (uint8_t i = 0; i < 4; i++) {
        rx_tx_addr[i] = lfsr & 0xff;
        rand32_r(&lfsr, i);
    }
    rx_tx_addr[4] = 0xa2;
}

void prhex(uint8_t a)
{
      if (a < 16) Serial.print('0');
      Serial.print(a,HEX);
}

void printpkt()
{
  Serial.print("pkt: ");
  for(uint8_t i = 0; i < PAYLOADSIZE; i++) prhex(packet[i]);
  Serial.println("");
}

void printaddr()
{
  Serial.print("addr: ");
  for(uint8_t i = 0; i < 5; i++) prhex(rx_tx_addr[4-i]);
  Serial.println("");
}

void printchn()
{
  Serial.print("chans: ");
  for(uint8_t i = 0; i < 4; i++) prhex(chans[i]);
  Serial.println("");
}

// channels determined by last byte (LSB) of tx address
// set_channels(rx_tx_addr[0] & 0x1f) - little endian
static void set_channels(uint8_t laddress) {
  static const uint8_t start_chans_1[] = {0x0a, 0x1a, 0x2a, 0x3a};
  static const uint8_t start_chans_2[] = {0x2a, 0x0a, 0x42, 0x22};
  static const uint8_t start_chans_3[] = {0x1a, 0x3a, 0x12, 0x32};

  uint32_t *pchans = (uint32_t *)chans;   // avoid compiler warning

  if (laddress < 0x10) {
    if (laddress == 6) laddress = 7;
    for(uint8_t i=0; i < 4; i++) chans[i] = start_chans_1[i] + laddress;
  } else if (laddress < 0x18) {
    for(uint8_t i=0; i < 4; i++)  chans[i] = start_chans_2[i] + (laddress & 0x07);
    if (laddress == 0x16) { chans[0]++; chans[1] ++; }
  } else if (laddress < 0x1e) {
    for(uint8_t i=0; i < 4; i++) chans[i] = start_chans_3[i] + (laddress & 0x07);
  } else if (laddress == 0x1e) {
      *pchans = 0x38184121;
  } else {
      *pchans = 0x39194121;
  }
  printchn();
  
}

static uint8_t checksum(uint8_t *data)
{
    uint8_t sum = data[0];

    for (int i=1; i < PAYLOADSIZE-1; i++)
            sum ^= data[i];
    
    return sum + 0x55;
}

static void build_packet(uint8_t bind) {
    if (bind) {
        packet[0] = rx_tx_addr[4];
        packet[1] = rx_tx_addr[3];
        packet[2] = rx_tx_addr[2];
        packet[3] = rx_tx_addr[1];
        packet[4] = rx_tx_addr[0];
        packet[5] = 0xaa;
        packet[6] = 0xaa;
        packet[7] = 0xaa;
        packet[8] = 0x00;
    } else {
        if (symax_proto_data!=NULL) memcpy(packet, (uint8_t*)symax_proto_data, 8);
        else memset(packet, 0, 8);
        packet[5] |= FLAG5_SET1;
        packet[8] = 0x00;
    }
    packet[9] = checksum(packet);
}

uint16_t symax_callback()
{
    switch (phase) {
    case SYMAX_INIT:
        tx_power = MODEL_tx_power;
        packet_counter = 0;

        NRF24L01_Initialize();

        NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknoledgement
        NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x00);  // 0x3F Enable all data pipes (even though not used?)
        NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, 8);

        NRF24L01_SetBitrate(NRF24L01_BR_250K);
        NRF24L01_SetPower(tx_power);

        NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
        NRF24L01_WriteReg(NRF24L01_17_FIFO_STATUS, 0x00); // Just in case, no real bits to write here

        memcpy(rx_tx_addr, (uint8_t*)"\xab\xac\xad\xae\xaf", 5);
        NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);

        NRF24L01_ReadReg(NRF24L01_07_STATUS);
        NRF24L01_SetTxRxMode(TX_EN);
        
        phase = SYMAX_INIT1;
	return INITIAL_WAIT;

    case SYMAX_INIT1:
        printaddr();
        initialize_rx_tx_addr();
        memcpy(chans, (uint8_t*)"\x4b\x30\x40\x20", 4);
        printchn();

        current_chan = 0;
        packet_counter = 0;
        phase = SYMAX_BIND2;
        counter = BIND_COUNT;
        return FIRST_PACKET_DELAY;

    case SYMAX_BIND2:
        build_packet(1);
        if (--counter == 0) phase = SYMAX_BIND3;
        goto send_pkt;

    case SYMAX_BIND3:
        printaddr();
        set_channels(rx_tx_addr[0] & 0x1f);
        NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);
        current_chan = 0;
        packet_counter = 0;
        phase = SYMAX_DATA;
        break;

    case SYMAX_DATA:
        build_packet(0);
    send_pkt:
        // clear packet status bits and TX FIFO
        NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
        NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x2e);
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, chans[current_chan]);
        NRF24L01_FlushTx();

        NRF24L01_WritePayload(packet, PAYLOADSIZE);

        // use each channel twice
        if (packet_counter++ % 2) current_chan = (current_chan + 1) % 4;

        // Check and adjust transmission power. We do this after transmission to not bother
        // with timeout after power settings change -  we have plenty of time until next packet.
        if (tx_power != MODEL_tx_power)
            NRF24L01_SetPower(tx_power = MODEL_tx_power);
        break;
    }
    return PACKET_PERIOD;
}

uint8_t symax_binding()
{
    return (phase != SYMAX_DATA);
}

void symax_data(struct SymaXData *x)
{
    symax_proto_data = x;
}
