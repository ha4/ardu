/*
  This project is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Multiprotocol is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Multiprotocol.  If not, see <http://www.gnu.org/licenses/>.
*/
// Compatible with EAchine H8 mini, H10, BayangToys X6/X7/X9, JJRC JJ850 ...
// Last sync with hexfet new_protocols/bayang_nrf24l01.c dated 2015-12-22

#include <Arduino.h>
#include "iface_nrf24l01.h"
#include "interface.h"

#define DEBUG

#ifdef DEBUG
#define DEBUGLN(x) Serial.println(x)
#else
#define DEBUGLN(x)
#endif

#define BAYANG_BIND_COUNT		1000
#define BAYANG_PACKET_PERIOD	2000
#define BAYANG_PACKET_TELEM_PERIOD	5000
#define BAYANG_INITIAL_WAIT		500
#define BAYANG_PACKET_SIZE		15
#define BAYANG_RF_NUM_CHANNELS	4
#define BAYANG_RF_BIND_CHANNEL	0
#define BAYANG_RF_BIND_CHANNEL_X16_AH 10
#define BAYANG_ADDRESS_LENGTH	5

enum {
  BAYANG_BIND = 0,
  BAYANG_WRITE,
  BAYANG_CHECK,
  BAYANG_READ,
};

#define BAYANG_CHECK_DELAY    1000    // Time after write phase to check write complete
#define BAYANG_READ_DELAY   600     // Time before read phase

uint8_t  rx_tx_addr[5];
uint8_t  rx_id[5];
uint8_t  phase;
uint8_t  proto_flags;
uint8_t  hopping_frequency[BAYANG_RF_NUM_CHANNELS];
uint8_t  hopping_frequency_no = 0;
uint8_t  rf_ch_num;
uint8_t  packet[BAYANG_PACKET_SIZE];
uint16_t bind_counter;
uint16_t tele_counter;
uint8_t  packet_count;
uint16_t *proto_data = NULL;

static uint8_t BAYANG_sum()
{
  uint8_t c = packet[0];
  for (uint8_t i = 1; i < BAYANG_PACKET_SIZE - 1; i++)
    c += packet[i];
  return c;
}

static void __attribute__((unused)) BAYANG_send_packet()
{
  uint8_t i;
  if (phase == BAYANG_BIND) {
    packet[0] = 0xA4;
    for (i = 0; i < 5; i++)
      packet[i + 1] = rx_tx_addr[i];
    for (i = 0; i < 4; i++)
      packet[i + 6] = hopping_frequency[i];
    packet[10] = rx_tx_addr[0];	// txid[0]
    packet[11] = rx_tx_addr[1];	// txid[1]
  } else {
    uint16_t val;
    packet[0] = 0xA5;
    packet[1] = 0xFA;		// normal mode is 0xF7, expert 0xFa , D4 normal is 0xF4
  }
  packet[12] = rx_tx_addr[2];	// txid[2]
  packet[13] = 0x0A;
  packet[14] = BAYANG_sum();

  NRF24L01_WriteReg(NRF24L01_05_RF_CH, phase==BAYANG_BIND ? rf_ch_num : hopping_frequency[hopping_frequency_no]);
  if(++hopping_frequency_no >= BAYANG_RF_NUM_CHANNELS)
    hopping_frequency_no=0;

  // Power on, TX mode, 2byte CRC
  // Why CRC0? xn297 does not interpret it - either 16-bit CRC or nothing
  NRF24L01_FlushTx();
  NRF24L01_SetTxRxMode(TX_EN);
  XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
  XN297_WritePayload(packet, BAYANG_PACKET_SIZE);

  //	NRF24L01_SetPower();	// Set tx_power
}

static void __attribute__((unused)) BAYANG_check_rx(void)
{
  if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR)) { // data received from model
    XN297_ReadPayload(packet, BAYANG_PACKET_SIZE);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 255);

    NRF24L01_FlushRx();
    // decode data , check sum is ok as well, since there is no crc
    if (packet[0] == 0x85 && packet[14] == BAYANG_sum()) {
      //      v_lipo1 = (packet[3]<<7) + (packet[4]>>1); // uncompensated battery volts*100/2
      //      v_lipo2 = (packet[5]<<7) + (packet[6]>>1); // compensated battery volts*100/2
      //      RX_LQI = packet[7]; // reception in packets / sec
      //      RX_RSSI = RX_LQI;
      //Flags
      //uint8_t flags = packet[3] >> 3;
      // battery low: flags & 1
      ///      telemetry_counter++;
      ///      if(telemetry_lost==0)
      ///        telemetry_link=1;
    }
  }
  NRF24L01_SetTxRxMode(TXRX_OFF);
}

static void __attribute__((unused)) BAYANG_init_nrf()
{
  NRF24L01_Initialize();
  NRF24L01_SetTxRxMode(TX_EN);

  XN297_SetTXAddr((uint8_t *)"\x00\x00\x00\x00\x00", BAYANG_ADDRESS_LENGTH);

  NRF24L01_FlushTx();
  NRF24L01_FlushRx();
  NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     	// Clear data ready, data sent, and retransmit
  NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      	// No Auto Acknowldgement on all data pipes
  NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  	// Enable data pipe 0 only
  NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, BAYANG_PACKET_SIZE);
  NRF24L01_SetBitrate(NRF24L01_BR_1M);             	// 1Mbps
  NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00);	// No retransmits
  NRF24L01_SetPower(TXPOWER_100mW);
  NRF24L01_Activate(0x73);							// Activate feature register
  NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);			// Disable dynamic payload length on all pipes
  NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);
  NRF24L01_Activate(0x73);

  rf_ch_num = BAYANG_RF_BIND_CHANNEL;
  proto_flags = 0;
}

// x=value from 1000 to 2000, 1500 middle
// t=trim from -31 to 31, 0 middle
void BAYANG_enc(int n, int x, int t)
{
  x=x-1500+0x1FF;
  if (x<0) x=0;
  if (x>1023)x=1023;
  t+=0x1F;
  if (t<0)t=0;
  if (t>63)t=63;
  packet[n+1]=x&0xff;
  packet[n]=(x>>8) + (t<<2);
}
void  BAYANG_bildchannels(uint16_t *AETR1234val)
{
  int val;
  //Flags packet[2]
  packet[2] = 0x00;
  packet[2] = BAYANG_FLAG_FLIP;
  packet[2] |= BAYANG_FLAG_RTH;
  packet[2] |= BAYANG_FLAG_PICTURE;
  packet[2] |= BAYANG_FLAG_VIDEO;
  packet[2] |= BAYANG_FLAG_HEADLESS;
  //Flags packet[3]
  packet[3] = 0x00;
  packet[3] = BAYANG_FLAG_INVERTED;
  packet[3] |= BAYANG_FLAG_TAKE_OFF;
  packet[3] |= BAYANG_FLAG_EMG_STOP;
  
  BAYANG_enc(4,AETR1234val[0],0); //Aileron
  BAYANG_enc(6,AETR1234val[1],0); //Elevator
  BAYANG_enc(8,AETR1234val[2],0); //Throttle
  BAYANG_enc(10,AETR1234val[3],0);//Rudder
}

uint16_t BAYANG_callback()
{
  switch(phase) {
    case BAYANG_BIND:
      if(--bind_counter == 0) {
        XN297_SetTXAddr(rx_tx_addr, BAYANG_ADDRESS_LENGTH);
        phase++;	// bind done ->WRITE
        DEBUGLN("bind complete");
      }else
        BAYANG_send_packet();
      break;
    case BAYANG_WRITE:
      if (proto_data != NULL) BAYANG_bildchannels(proto_data);
      BAYANG_send_packet();
      if(proto_flags & BAYANG_OPTION_TELEMETRY) {
        if(++tele_counter > 200) {
          tele_counter = 0;
          //telemetry reception packet rate - packets per second
          //TX_LQI = telemetry_counter >> 1;
          //telemetry_counter = 0;
          //telemetry_lost = 0;
        }
        phase++;  // -> CHECK
        return BAYANG_CHECK_DELAY;
      }
      break;
    case BAYANG_CHECK:
      // switch radio to rx as soon as packet is sent
      uint16_t start = (uint16_t)micros();
      while ((uint16_t)((uint16_t)micros() - (uint16_t)start) < 1000)   // Wait max 1ms
        if ((NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_TX_DS)))
          break;
      NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x03);
      phase++;  // READ
      return BAYANG_PACKET_TELEM_PERIOD - BAYANG_CHECK_DELAY - BAYANG_READ_DELAY;
    case BAYANG_READ:
      BAYANG_check_rx();
      phase = BAYANG_WRITE;
      return BAYANG_READ_DELAY;
  }
  return BAYANG_PACKET_PERIOD;
}


// Convert 32b id to rx_tx_addr
void set_rx_tx_addr(uint32_t id)
{ // Used by almost all protocols
  rx_tx_addr[0] = (id >> 24) & 0xFF;
  rx_tx_addr[1] = (id >> 16) & 0xFF;
  rx_tx_addr[2] = (id >>  8) & 0xFF;
  rx_tx_addr[3] = (id >>  0) & 0xFF;
  rx_tx_addr[4] = (rx_tx_addr[2] & 0xF0) | (rx_tx_addr[3] & 0x0F);
}

static void __attribute__((unused)) BAYANG_initialize_txid()
{
  //Could be using txid[0..2] but using rx_tx_addr everywhere instead...
  hopping_frequency[0] = 0;
  hopping_frequency[1] = (rx_tx_addr[3] & 0x1F) + 0x10;
  hopping_frequency[2] = hopping_frequency[1] + 0x20;
  hopping_frequency[3] = hopping_frequency[2] + 0x20;
  hopping_frequency_no = 0;
}

void BAYANG_bind()
{
  phase = BAYANG_BIND; // autobind protocol
  bind_counter = BAYANG_BIND_COUNT;
}

uint16_t initBAYANG(void)
{
  BAYANG_initialize_txid();
  BAYANG_init_nrf();
  BAYANG_bind();
  packet_count = 0;
  tele_counter = 0;
  return BAYANG_INITIAL_WAIT + BAYANG_PACKET_PERIOD;
}

void BAYANG_data(uint16_t *AETR1234)
{
  proto_data = AETR1234;
}

void BAYANG_telemetry(uint16_t *tele)
{

}
