#include <Arduino.h>
#include "bayang_rx.h"
#include "nRF24L01.h"
#include "XN297_emu.h"

//#define BIND_DEBUG
//#define BAYANG_AUTOBIND

#define BAYANG_BIND_COUNT       5000  /* 5 seconds */
#define BAYANG_PACKET_PERIOD    2000
#define BAYANG_PACKET_TELEM_PERIOD  5000
#define BAYANG_INITIAL_WAIT   500

#define BAYANG_PACKET_SIZE      15
#define BAYANG_RF_NUM_CHANNELS  4
#define BAYANG_RF_BIND_CHANNEL  0
#define BAYANG_ADDRESS_LENGTH   5
#define BAYANG_TELEMETRY_CHANS  8

#define BAYANG_RX_EEPROM_OFFSET  762   // (5) TX ID + (4) channels, 9 bytes, end is 771 

static uint8_t rf_chan = 0;
static uint8_t rf_channels[BAYANG_RF_NUM_CHANNELS] = {0,};
static uint8_t rx_tx_addr[BAYANG_ADDRESS_LENGTH];
static uint8_t bayang_type; /* bind packet type 0xA4:normal, A2:analog aux, A3:telemetry, A1:telemetry+analog aux */
static uint32_t lastRxTime;
static bool rx_received, rx_available;
static int8_t slip_retry;
static uint16_t bind_counter;
static uint32_t pps_timer;
static uint16_t pps_counter, rx_lqi;
static uint8_t packet[32]; /* [0] 0xA5 - data packet, 0xA6 X16_AH,IRDRONE data */
static uint8_t phase;
static struct BayangData *bdata = NULL;

enum {
  BAYANG_RX_BINDING = 0,
  BAYANG_RX_BIND,
  BAYANG_RX_DATA,
  BAYANG_RX_DATA_LOSS,
};

#define EE_ADDR uint8_t*

void BAYANG_init_rf()
{
  const uint8_t bind_address[BAYANG_ADDRESS_LENGTH] = {0, 0, 0, 0, 0};

  XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
  XN297_SetTXAddr(bind_address, BAYANG_ADDRESS_LENGTH);
  XN297_SetRXAddr(bind_address, BAYANG_PACKET_SIZE);
  XN297_RFChannel(BAYANG_RF_BIND_CHANNEL);
  XN297_SetTxRxMode(TXRX_OFF);
  XN297_SetTxRxMode(RX_EN);
}

bool validPacket()
{
  uint8_t a = packet[0];
  for (int i = 1; i < BAYANG_PACKET_SIZE-1; i++)
    a += packet[i];
  return a == packet[14];
}

void BAYANG_RX_decode(struct BayangData* data)
{
  data->roll = (packet[4] & 0x03) * 256 + packet[5];
  data->pitch = (packet[6] & 0x03) * 256 + packet[7];
  data->throttle = (packet[8] & 0x03) * 256 + packet[9];
  data->yaw = (packet[10] & 0x03) * 256 + packet[11];

  data->trims[0] = packet[6] >> 2;
  data->trims[1] = packet[4] >> 2;
  data->trims[2] = packet[8] >> 2;
  data->trims[3] = packet[10] >> 2;
  data->trims[0] -= 0x1f;
  data->trims[1] -= 0x1f;
  data->trims[2] -= 0x1f;
  data->trims[3] -= 0x1f;

  if(data->option & BAYANG_OPTION_ANALOGAUX) {
    data->aux1=packet[1];
    data->aux2=packet[13];
  } else
    data->aux1 = (packet[1] == 0xfa) ? 255 : 0;
  data->flags2=packet[2];
  data->flags3=packet[3];
  
  rx_available = true;
}

void BAYANG_RX_telemetry()
{
  uint32_t bits = 0;
  uint8_t bitsavailable = 0;
  uint8_t idx = 0;

  packet[idx++] = rx_lqi;
  packet[idx++] = rx_lqi>>1; // no RSSI: 125..0
  packet[idx++] = 0;     // start channel
  packet[idx++] = BAYANG_TELEMETRY_CHANS;      // number of channels in packet

  // convert & pack channels
  for (uint8_t i = 0; i < packet[3]; i++) {
    uint16_t val = 192; // min
    if (i < 4) { // AETR
      val=((val+128)<<3)/5;
    } else if (i == 4 || i == 5) { // analog AUX
      val=((val+32)<<5)/5;
    } else if ((i >= 6) && val) {  // discrete
      val = 1792;
    }
    bits |= (uint32_t)val << bitsavailable;
    bitsavailable += 11;
    while (bitsavailable >= 8) {
      packet[idx++] = bits & 0xff;
      bits >>= 8;
      bitsavailable -= 8;
    }
  }
  if (bitsavailable > 0) packet[idx++]=bits&0xff;
}

uint8_t BAYANG_RX_available()
{
  if(rx_available) { rx_available=false; return true; }
  return false;
}

void bind_type()
{
  if (bdata==NULL) return;
  bdata->option=0; /* 0xA4:normal, A2:analog aux, A3:telemetry, A1:telemetry+analog aux */
  if (bayang_type==0xA2 || bayang_type==0xA1) bdata->option|=BAYANG_OPTION_ANALOGAUX;
  if (bayang_type==0xA3 || bayang_type==0xA1) bdata->option|=BAYANG_OPTION_TELEMETRY;
}

void BAYANG_eeprom_save()
{
  uint16_t temp;
  temp = BAYANG_RX_EEPROM_OFFSET;
  for (int i = 0; i < BAYANG_ADDRESS_LENGTH; i++) {
      rx_tx_addr[i] = packet[i + 1];
      eeprom_write_byte((EE_ADDR)temp++, rx_tx_addr[i]);
  }
  for (int i = 0; i < BAYANG_RF_NUM_CHANNELS; i++) {
      rf_channels[i] = packet[i + 6];
      eeprom_write_byte((EE_ADDR)temp++, rf_channels[i]);
  }
  bayang_type=packet[0];
  eeprom_write_byte((EE_ADDR)temp++, bayang_type);
}

void BAYANG_eeprom()
{
  uint16_t temp;
  temp = BAYANG_RX_EEPROM_OFFSET;
  // load
  for (int i = 0; i < BAYANG_ADDRESS_LENGTH; i++)
      rx_tx_addr[i] = eeprom_read_byte((EE_ADDR)temp++);
  for (int i = 0; i < BAYANG_RF_NUM_CHANNELS; i++)
      rf_channels[i] = eeprom_read_byte((EE_ADDR)temp++);
  bayang_type=eeprom_read_byte((EE_ADDR)temp++);

  bind_type();
  XN297_SetTXAddr(rx_tx_addr, BAYANG_ADDRESS_LENGTH);
  XN297_SetRXAddr(rx_tx_addr, BAYANG_PACKET_SIZE);
}

/*
 *   Interface part
 */

void BAYANG_RX_data(struct BayangData *t)
{
  bdata = t;
}

void BAYANG_RX_init()
{
  BAYANG_init_rf();
  if (bdata && !(bdata->option & BAYANG_OPTION_BIND)) {
    BAYANG_eeprom();
    phase = BAYANG_RX_DATA;
  } else {
    phase = BAYANG_RX_BINDING;
    bind_counter=BAYANG_BIND_COUNT;
  }
  rf_chan = 0;
  rx_received = false;
}

void BAYANG_RX_bind()
{
  BAYANG_RX_init();
  phase = BAYANG_RX_BINDING;
  bind_counter=BAYANG_BIND_COUNT;
}

int BAYANG_RX_state() // 0:binding 1:data -1:data loss
{
  if (phase==BAYANG_RX_DATA) return 1;
  if (phase==BAYANG_RX_DATA_LOSS) return -1;
  return 0;
}

uint16_t BAYANG_RX_callback()
{
  switch (phase) {
    case BAYANG_RX_BINDING:
      if (bind_counter) bind_counter--;
      else {
        BAYANG_eeprom();
        XN297_SetTxRxMode(RX_EN);
        phase = BAYANG_RX_DATA;
        break;
      }
      if(XN297_IsRX()) {
        if(XN297_ReadPayload(packet, BAYANG_PACKET_SIZE) && validPacket() && (packet[0] >= 0xA1 && packet[0] <= 0xA4)) {
          BAYANG_eeprom_save();
          XN297_SetTXAddr(rx_tx_addr, BAYANG_ADDRESS_LENGTH);
          XN297_SetRXAddr(rx_tx_addr, BAYANG_PACKET_SIZE);
          bind_type();
          phase = BAYANG_RX_DATA;
        }
        XN297_SetTxRxMode(RX_EN);
      }
    break;

    case BAYANG_RX_DATA:
    case BAYANG_RX_DATA_LOSS:
      if (XN297_IsRX()) {
        XN297_ReadPayload(packet, BAYANG_PACKET_SIZE);
        if (validPacket() &&  packet[0] == 0xA5) {
          if (bdata) BAYANG_RX_decode(bdata);
          phase = BAYANG_RX_DATA;
          rx_received = true;
          slip_retry = 8;
          pps_counter++;
        }
      }
      // pps count
      if (millis() - pps_timer >= 1000) {
        pps_timer = millis();
        rx_lqi = pps_counter >> 1;
        if(bdata) bdata->lqi = rx_lqi;
        pps_counter = 0;
      }
      // next hopping channel
      if (slip_retry++ >= 8) {  
        rf_chan++;
        if (rf_chan >= BAYANG_RF_NUM_CHANNELS) rf_chan = 0;
        XN297_Hopping(rf_channels, rf_chan);
        XN297_SetTxRxMode(RX_EN);

        if (phase==BAYANG_RX_DATA) {
          if (rx_received) { // in sync
            rx_received = false;
            slip_retry = 1;
            return BAYANG_PACKET_PERIOD - BAYANG_PACKET_PERIOD / 4; // 1500us
          } else { // lost packet
            slip_retry = 0;
            if (rx_lqi == 0) phase = BAYANG_RX_DATA_LOSS;
          }
        } else
          slip_retry = -16; // retry longer until first packet is caught
      }
      return BAYANG_PACKET_PERIOD / 8; // 250 us
  }
  return BAYANG_PACKET_PERIOD / 2;
}
