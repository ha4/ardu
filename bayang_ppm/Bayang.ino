/*
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License.
  If not, see <http://www.gnu.org/licenses/>.
*/

//#define BIND_DEBUG
//#define BAYANG_AUTOBIND

#define BAYANG_BIND_COUNT       1000
#define BAYANG_PACKET_PERIOD    2000
#define BAYANG_PACKET_TELEM_PERIOD  5000
#define BAYANG_INITIAL_WAIT   500
#define BAYANG_PACKET_SIZE      15
#define BAYANG_RF_NUM_CHANNELS  4
#define BAYANG_RF_BIND_CHANNEL  0
#define BAYANG_ADDRESS_LENGTH   5

#define BAYANG_RX_EEPROM_OFFSET  762   // (5) TX ID + (4) channels, 9 bytes, end is 771 

#define AUXNUMBER 5
#define CH_FLIP 0
#define CH_EXPERT 1
#define CH_HEADFREE 2
#define CH_RTH 3
#define CH_INV 4

static uint8_t rf_chan = 0;
static uint8_t rf_channels[BAYANG_RF_NUM_CHANNELS] = {0,};
static uint8_t rx_tx_addr[BAYANG_ADDRESS_LENGTH];
char aux[AUXNUMBER];
uint32_t lastRxTime;
bool rx_received, rx_available;
int8_t slip_retry;
uint16_t bind_counter;
uint32_t pps_timer;
uint16_t pps_counter, rx_lqi;
uint8_t packet[32];
uint8_t phase;

enum {
  // flags going to packet[2]
  BAYANG_FLAG_RTH      = 0x01,
  BAYANG_FLAG_HEADLESS = 0x02,
  BAYANG_FLAG_FLIP     = 0x08,
  BAYANG_FLAG_VIDEO    = 0x10,
  BAYANG_FLAG_SNAPSHOT = 0x20,
};

enum {
  // flags going to packet[3]
  BAYANG_FLAG_INVERT   = 0x80,
};

enum {
  BAYANG_RX_BINDING = 0,
  BAYANG_RX_BIND,
  BAYANG_RX_DATA,
  BAYANG_RX_DATA_LOSS,
};

#define EE_ADDR uint8_t*

MyData *bdata = NULL;

bool validPacket()
{
  int a = 0;
  for (int i = 0; i < 14; i++)
    a += packet[i];

  return (a & 0xFF) == packet[14];
}

void BAYANG_init_nrf()
{
  const u8 bind_address[] = {0, 0, 0, 0, 0};

  SPI_Begin();
  NRF24L01_Reset();
  NRF24L01_Initialize();
  XN297_SetRXAddr(bind_address, BAYANG_ADDRESS_LENGTH);
  XN297_SetTXAddr(bind_address, BAYANG_ADDRESS_LENGTH);
  NRF24L01_FlushTx();
  NRF24L01_FlushRx();
  NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);

  NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
  NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01); //enable pipe 0
  NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, BAYANG_PACKET_SIZE + 2); // rx pipe 0 set data size
  NRF24L01_WriteReg(NRF24L01_05_RF_CH, BAYANG_RF_BIND_CHANNEL);
 // NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00); // no retransmits
  NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps
  NRF24L01_SetPower(RF_POWER);                   // set power amp power
  NRF24L01_Activate(0x73);                         // Activate feature register
  NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);      // Disable dynamic payload length on all pipes
  NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);
  NRF24L01_Activate(0x73);
  NRF24L01_SetTxRxMode(TXRX_OFF);
  
  NRF24L01_FlushRx();
  NRF24L01_SetTxRxMode(RX_EN);
  XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP) | _BV(NRF24L01_00_PRIM_RX));
}

uint8_t BAYANG_RX_available()
{
  if(rx_available) { rx_available=false; return true; }
  return false;
}

void BAYANG_RX_data(MyData *t)
{
  bdata = t;
}

void BAYANG_decode(MyData* data)
{
  data->roll = (packet[4] & 0x03) * 256 + packet[5];
  data->pitch = (packet[6] & 0x03) * 256 + packet[7];
  data->yaw = (packet[10] & 0x03) * 256 + packet[11];
  data->throttle = (packet[8] & 0x03) * 256 + packet[9];

  char trims[4];
  trims[0] = packet[6] >> 2;
  trims[1] = packet[4] >> 2;
  trims[2] = packet[8] >> 2;
  trims[3] = packet[10] >> 2;

  aux[CH_INV] = (packet[3] & 0x80) ? 1 : 0; // inverted flag
  aux[CH_FLIP] = (packet[2] & 0x08) ? 1 : 0;
  data-> aux1 = aux[CH_EXPERT] = (packet[1] == 0xfa) ? 1 : 0;
  aux[CH_HEADFREE] = (packet[2] & 0x02) ? 1 : 0;
  aux[CH_RTH] = (packet[2] & 0x01) ? 1 : 0; // rth channel

   rx_available = true;
}

void BAYANG_eeprom(bool save)
{
  uint16_t temp;
  temp = BAYANG_RX_EEPROM_OFFSET;
  if (save) {
    for (int i = 0; i < BAYANG_ADDRESS_LENGTH; i++) {
      rx_tx_addr[i] = packet[i + 1];
      eeprom_write_byte((EE_ADDR)temp++, rx_tx_addr[i]);
    }
    for (int i = 0; i < BAYANG_RF_NUM_CHANNELS; i++) {
      rf_channels[i] = packet[i + 6];
      eeprom_write_byte((EE_ADDR)temp++, rf_channels[i]);
    }
  } else { // load
    for (int i = 0; i < BAYANG_ADDRESS_LENGTH; i++)
      rx_tx_addr[i] = eeprom_read_byte((EE_ADDR)temp++);
    for (int i = 0; i < BAYANG_RF_NUM_CHANNELS; i++)
      rf_channels[i] = eeprom_read_byte((EE_ADDR)temp++);
  }

#ifdef BIND_DEBUG
  Serial.print("bind-info:");
  for(int i = 0; i < BAYANG_ADDRESS_LENGTH; i++)
    { Serial.print(rx_tx_addr[i],HEX); Serial.print(' '); }
  for(int i = 0; i < BAYANG_RF_NUM_CHANNELS; i++)
    { Serial.print(rf_channels[i],HEX); Serial.print(' '); }
  Serial.println();
#endif

  XN297_SetTXAddr(rx_tx_addr, BAYANG_ADDRESS_LENGTH);
  XN297_SetRXAddr(rx_tx_addr, BAYANG_ADDRESS_LENGTH);
}

uint16_t BAYANG_RX_init()
{
  BAYANG_init_nrf();
#ifdef BAYANG_AUTOBIND
  phase = BAYANG_RX_BINDING;
#else
  phase = BAYANG_RX_BIND;
#endif
  rf_chan = 0;
  rx_received = false;
  bind_counter=0;

  return BAYANG_PACKET_PERIOD / 2;
}

uint16_t BAYANG_RX_bind()
{
  uint16_t t=BAYANG_RX_init();
  phase = BAYANG_RX_BINDING;
  return t;
}

int BAYANG_RX_state()
{
  if (phase==BAYANG_RX_DATA) return 1;
  if (phase==BAYANG_RX_DATA_LOSS) return -1;
  return 0;
}

uint16_t BAYANG_RX_callback()
{
  switch (phase) {
    case BAYANG_RX_BINDING:
      if(NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR)) {
        XN297_ReadPayload(packet, BAYANG_PACKET_SIZE);

        if(validPacket() && (packet[0] == 0xA4 || packet[0] == 0xA2)) {
#ifdef BIND_DEBUG
          Serial.println("got bind packet");
#endif
          BAYANG_eeprom(true);
          phase = BAYANG_RX_DATA;
        }
        NRF24L01_FlushRx();
        NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
      }
      if (bind_counter++ >= BAYANG_BIND_COUNT*2)
        phase=BAYANG_RX_BIND;
      break;

    case BAYANG_RX_BIND:
#ifdef BIND_DEBUG
      Serial.println("read bind");
#endif
      BAYANG_eeprom(false);
      NRF24L01_FlushRx();
      NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
      phase = BAYANG_RX_DATA;
      break;

    case BAYANG_RX_DATA:
    case BAYANG_RX_DATA_LOSS:
      if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR)) {
        XN297_ReadPayload(packet, BAYANG_PACKET_SIZE);
        if (validPacket() &&  packet[0] == 0xA5) {
          if (bdata) BAYANG_decode(bdata);
          phase = BAYANG_RX_DATA;
          rx_received = true;
          slip_retry = 8;
          pps_counter++;
        }
      }
      // pps count
      if (millis() - pps_timer >= 1000) {
        pps_timer = millis();
        //Serial.print("pps counter:");  Serial.println(pps_counter);
        rx_lqi = pps_counter >> 1;
        pps_counter = 0;
      }
      // next hopping channel
      if (slip_retry++ >= 8) {
        rf_chan++;
        if (rf_chan >= BAYANG_RF_NUM_CHANNELS) rf_chan = 0;
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_channels[rf_chan]);
        NRF24L01_FlushRx();
        NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
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
          slip_retry = -8;
      }
      return BAYANG_PACKET_PERIOD / 8; // 250 us
  }
  return BAYANG_PACKET_PERIOD / 2;
}
