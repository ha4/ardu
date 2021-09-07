#include <Arduino.h>
#include "nRF24L01.h"
#include "XN297_emu.h"

static uint8_t xn297_crc;
static uint8_t xn297_scramble_en;
static uint8_t xn297_bitrate;
static uint8_t xn297_rfchip;
static uint8_t xn297_addr_len, xn297_rx_packet_len;
static uint8_t xn297_rx_len;
static uint8_t xn297_tx_addr[5];
static uint8_t xn297_rx_addr[5];

static const uint8_t xn297_scramble[] = {
    0xe3, 0xb1, 0x4b, 0xea, 0x85, 0xbc, 0xe5, 0x66,
    0x0d, 0xae, 0x8c, 0x88, 0x12, 0x69, 0xee, 0x1f,
    0xc7, 0x62, 0x97, 0xd5, 0x0b, 0x79, 0xca, 0xcc,
    0x1b, 0x5d, 0x19, 0x10, 0x24, 0xd3, 0xdc, 0x3f,
    0x8e, 0xc5, 0x2f};
    
static const uint16_t PROGMEM xn297_crc_xorout_scrambled[] = {
    0x0000, 0x3448, 0x9BA7, 0x8BBB, 0x85E1, 0x3E8C, 
    0x451E, 0x18E6, 0x6B24, 0xE7AB, 0x3828, 0x814B,
    0xD461, 0xF494, 0x2503, 0x691D, 0xFE8B, 0x9BA7,
    0x8B17, 0x2920, 0x8B5F, 0x61B1, 0xD391, 0x7401, 
    0x2138, 0x129F, 0xB3A0, 0x2988};

const uint16_t PROGMEM xn297_crc_xorout[] = {
    0x0000, 0x3D5F, 0xA6F1, 0x3A23, 0xAA16, 0x1CAF,
    0x62B2, 0xE0EB, 0x0821, 0xBE07, 0x5F1A, 0xAF15,
    0x4F0A, 0xAD24, 0x5E48, 0xED34, 0x068C, 0xF2C9,
    0x1852, 0xDF36, 0x129D, 0xB17C, 0xD5F5, 0x70D7,
    0xB798, 0x5133, 0x67DB, 0xD94E, 0x0A5B, 0xE445,
    0xE6A5, 0x26E7, 0xBDAB, 0xC379, 0x8E20 };

const uint16_t PROGMEM xn297_crc_xorout_scrambled_enhanced[] = {
    0x0000, 0x7EBF, 0x3ECE, 0x07A4, 0xCA52, 0x343B,
    0x53F8, 0x8CD0, 0x9EAC, 0xD0C0, 0x150D, 0x5186,
    0xD251, 0xA46F, 0x8435, 0xFA2E, 0x7EBD, 0x3C7D,
    0x94E0, 0x3D5F, 0xA685, 0x4E47, 0xF045, 0xB483,
    0x7A1F, 0xDEA2, 0x9642, 0xBF4B, 0x032F, 0x01D2,
    0xDC86, 0x92A5, 0x183A, 0xB760, 0xA953 };


uint8_t bit_reverse(uint8_t b_in)
{
  b_in = (b_in << 4) | (b_in >> 4);
  b_in = ((b_in << 2)&0xCC) | ((b_in >> 2)&0x33);
  return ((b_in << 1)&0xAA) | ((b_in >> 1)&0x55);
    //uint8_t b_out = 0; for (uint8_t i = 0; i < 8; ++i,  b_in >>= 1) b_out = (b_out << 1) | (b_in & 1);
    //return b_out;
}

static const uint16_t polynomial = 0x1021;
static const uint16_t initial    = 0xb5d2;
uint16_t crc16_update(uint16_t crc, unsigned char a, uint8_t bits=8)
{
    crc ^= a << 8;
    for (uint8_t i = 0; i < bits; ++i)
        crc = (crc << 1) ^ ((crc & 0x8000) ? polynomial : 0);
    return crc;
}

uint8_t XN297_Configure(uint8_t crc_en, uint8_t scramble_en, uint8_t speed250k)
{
  xn297_crc = crc_en;
  xn297_scramble_en = scramble_en;
  xn297_bitrate = speed250k;
  xn297_rfchip = XN297_NRF;

  NRF24L01_Initialize();
  NRF24L01_Reset();
  NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
  NRF24L01_FlushTx();
  NRF24L01_FlushRx();
  NRF24L01_WriteReg(NRF24L01_01_EN_AA,    0x00);  // No Auto Acknowldgement on all data pipes
  NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR,  0x01);  // Enable data pipe 0 only
  NRF24L01_WriteReg(NRF24L01_03_SETUP_AW,   0x03);  // 5 bytes rx/tx address
  NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00);  // no retransmits
  if(speed250k == XN297_250K)
    NRF24L01_SetBitrate(NRF24L01_BR_250K);
  else
    NRF24L01_SetBitrate(NRF24L01_BR_1M);              // 1Mbps
  NRF24L01_Activate(0x73);                         // Activate feature register
  NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);         // Disable dynamic payload length on all pipes
  NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);       // Set feature bits off and enable the command NRF24L01_B0_TX_PYLD_NOACK
  NRF24L01_Activate(0x73);
  NRF24L01_SetPower(0);
  NRF24L01_SetTxRxMode(TXRX_OFF);            // Clear data ready, data sent, retransmit and enable CRC 16bits, ready for TX

  return 1;
}

void XN297_Configure0(uint8_t flags)
{
    xn297_crc = !!(flags & _BV(NRF24L01_00_EN_CRC));
    flags &= ~(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO));
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, flags);
}


void XN297_SetTXAddr(const uint8_t* addr, uint8_t len)
{
    if (len > 5) len = 5;
    if (len < 3) len = 3;
    uint8_t buf[] = { 0x55, 0x0F, 0x71, 0x0C, 0x00 }; // bytes for XN297 preamble 0xC710F55 (28 bit)
    xn297_addr_len = len;
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, len-2);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, xn297_addr_len == 3 ? buf+1 : buf, 5);
    // Receive address is complicated. We need to use scrambled actual address as a receive address
    // but the TX code now assumes fixed 4-byte transmit address for preamble. We need to adjust it
    // first. Also, if the scrambled address begins with 1 nRF24 will look for preamble byte 0xAA
    // instead of 0x55 to ensure enough 0-1 transitions to tune the receiver. Still need to experiment
    // with receiving signals.
    memcpy(xn297_tx_addr, addr, len);
}

void XN297_SetRXAddr(const uint8_t* addr, uint8_t packet_len)
{
    for (uint8_t i = 0; i < xn297_addr_len; ++i) {
      xn297_rx_addr[i] = addr[i];
      if(xn297_scramble_en)
        xn297_rx_addr[i] ^= xn297_scramble[xn297_addr_len-i-1];
    }
    if(xn297_crc) packet_len += 2;    // Include CRC
    packet_len += 2;
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, xn297_rx_addr, xn297_addr_len);
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, packet_len >= 32 ? 32 : packet_len);
    xn297_rx_packet_len = packet_len;
}

void XN297_SetTxRxMode(uint8_t state)
{
  static uint8_t cur_mode=TXRX_OFF;
  NRF24L01_WriteReg(NRF24L01_07_STATUS, (1 << NRF24L01_07_RX_DR)      //reset the flag(s)
                        | (1 << NRF24L01_07_TX_DS)
                        | (1 << NRF24L01_07_MAX_RT));
  if(state==TXRX_OFF) {
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0);             //PowerDown
    CE_off;
    return;
  }
  CE_off;
  if(state == TX_EN) {
    NRF24L01_FlushTx();
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, 1 << NRF24L01_00_PWR_UP);
  } else {
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, (1 << NRF24L01_00_PWR_UP)
                          | (1 << NRF24L01_00_PRIM_RX));  // RX
  }
  if(state != cur_mode) {
    //delayMicroseconds(130);
    cur_mode=state;
  }     
  CE_on;
}

void XN297_SendPayload(uint8_t* msg, uint8_t len)
{
      NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
      NRF24L01_FlushTx();
      NRF24L01_WritePayload(msg, len);
}

void XN297_WritePayload(uint8_t* msg, uint8_t len)
{
    uint8_t buf[32];
    uint8_t last = 0;

    // If address length (which is defined by receive address length)
    // is less than 4 the TX address can't fit the preamble, so the last
    // byte goes here
    if (xn297_addr_len < 4)
      buf[last++] = 0x55;
    // address
    for (uint8_t i = 0; i < xn297_addr_len; ++i) {
        buf[last] = xn297_tx_addr[xn297_addr_len-i-1];
        if(xn297_scramble_en) buf[last] ^= xn297_scramble[i];
        last++;
    }
    // payload
    for (uint8_t i = 0; i < len; ++i) {
        buf[last] = bit_reverse(msg[i]);
        if(xn297_scramble_en) buf[last] ^= xn297_scramble[xn297_addr_len+i];
        last++;
    }
    // crc
    if (xn297_crc) {
        uint8_t offset = xn297_addr_len < 4 ? 1 : 0;
        uint16_t crc = initial;
        for (uint8_t i = offset; i < last; ++i)
            crc = crc16_update(crc, buf[i]);
        if(xn297_scramble_en)
          crc ^= pgm_read_word(&xn297_crc_xorout_scrambled[xn297_addr_len - 3 + len]);
        else
          crc ^= pgm_read_word(&xn297_crc_xorout[xn297_addr_len - 3 + len]);
        buf[last++] = crc >> 8;
        buf[last++] = crc & 0xff;
    }
    // send packet
    XN297_SendPayload(buf, last);
}

void XN297_WriteEnhancedPayload(uint8_t* msg, uint8_t len, uint8_t noack)
{
  uint8_t buf[32];
  uint8_t scramble_index=0;
  uint8_t last = 0;
  static uint8_t pid=0;

  if (xn297_addr_len < 4)
      buf[last++] = 0x55;
  // address
  for (uint8_t i = 0; i < xn297_addr_len; ++i) {
    buf[last] = xn297_tx_addr[xn297_addr_len-i-1];
    if(xn297_scramble_en)
      buf[last] ^= xn297_scramble[scramble_index++];
    last++;
  }

  // pcf
  buf[last] = (len << 1) | (pid>>1);
  if(xn297_scramble_en)
    buf[last] ^= xn297_scramble[scramble_index++];
  last++;
  buf[last] = (pid << 7) | (noack << 6);

  // payload
  buf[last]|= bit_reverse(msg[0]) >> 2; // first 6 bit of payload
  if(xn297_scramble_en)
    buf[last] ^= xn297_scramble[scramble_index++];
  for (uint8_t i = 0; i < len-1; ++i) {
    last++;
    buf[last] = (bit_reverse(msg[i]) << 6) | (bit_reverse(msg[i+1]) >> 2);
    if(xn297_scramble_en)
      buf[last] ^= xn297_scramble[scramble_index++];
  }
  last++;
  buf[last] = bit_reverse(msg[len-1]) << 6; // last 2 bit of payload
  if(xn297_scramble_en)
    buf[last] ^= xn297_scramble[scramble_index++] & 0xc0;
  // crc
  if (xn297_crc) {
    uint8_t offset = xn297_addr_len < 4 ? 1 : 0;
    uint16_t crc = initial;
    for (uint8_t i = offset; i < last; ++i)
      crc = crc16_update(crc, buf[i]);

    crc=crc16_update(crc,buf[last] & 0xc0, 2);
    if (xn297_scramble_en)
      crc ^= pgm_read_word(&xn297_crc_xorout_scrambled_enhanced[xn297_addr_len-3+len]);
    //else
    //  crc ^= pgm_read_word(&xn297_crc_xorout_enhanced[xn297_addr_len - 3 + len]);

    buf[last++] |= (crc >> 8) >> 2;
    buf[last++] = ((crc >> 8) << 6) | ((crc & 0xff) >> 2);
    buf[last++] = (crc & 0xff) << 6;
  }
  pid++;
  if(pid>3)
    pid=0;

  // send packet
  XN297_SendPayload(buf, last);
}

uint8_t XN297_IsRX()
{
  return (NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR));
}

void XN297_ReceivePayload(uint8_t* msg, uint8_t len)
{
  if (xn297_crc)
    len += 2;                 // Include CRC 
  NRF24L01_ReadPayload(msg, len);     // Read payload and CRC 
}

uint8_t XN297_ReadPayload(uint8_t* msg, uint8_t len)
{ //!!! Don't forget if using CRC to do a +2 on the received packet length (NRF24L01_11_RX_PW_Px !!! or CC2500_06_PKTLEN)
  uint8_t buf[32];

  // Read payload
  XN297_ReceivePayload(buf, len);

  // Decode payload
  for(uint8_t i=0; i<len; i++)
  {
    uint8_t b_in=buf[i];
    if(xn297_scramble_en) b_in ^= xn297_scramble[i+xn297_addr_len];
    msg[i] = bit_reverse(b_in);
  }

  if (!xn297_crc)
    return 1;  // No CRC so OK by default...

  // Calculate CRC
  uint16_t crc = initial;
  //process address
  for (uint8_t i = 0; i < xn297_addr_len; ++i)
    crc=crc16_update(crc, xn297_rx_addr[xn297_addr_len-i-1]);
  //process payload
  for (uint8_t i = 0; i < len; ++i)
    crc=crc16_update(crc, buf[i]);
  //xorout
  if(xn297_scramble_en)
    crc ^= pgm_read_word(&xn297_crc_xorout_scrambled[xn297_addr_len - 3 + len]);
  else
    crc ^= pgm_read_word(&xn297_crc_xorout[xn297_addr_len - 3 + len]);
  //test
  if( (crc >> 8) == buf[len] && (crc & 0xff) == buf[len+1])
    return 1;  // CRC  OK
  return 0;   // CRC NOK
}

uint8_t XN297_ReadEnhancedPayload(uint8_t* msg, uint8_t len)
{ //!!! Don't forget do a +2 and if using CRC add +4 on any of the used NRF24L01_11_RX_PW_Px !!!
  uint8_t buffer[32];
  uint8_t pcf_size;             // pcf payload size

  // Read payload
  XN297_ReceivePayload(buffer, len+2);    // Read pcf + payload + CRC

  // Decode payload
  pcf_size = buffer[0];
  if(xn297_scramble_en)
    pcf_size ^= xn297_scramble[xn297_addr_len];
  pcf_size = pcf_size >> 1;
  for(int i=0; i<len; i++) {
    msg[i] = bit_reverse((buffer[i+1] << 2) | (buffer[i+2] >> 6));
    if(xn297_scramble_en)
      msg[i] ^= bit_reverse((xn297_scramble[xn297_addr_len+i+1] << 2) | 
                  (xn297_scramble[xn297_addr_len+i+2] >> 6));
  }

  if (!xn297_crc)
    return pcf_size;            // No CRC so OK by default...

  // Calculate CRC
  uint16_t crc = initial;
  //process address
  for (uint8_t i = 0; i < xn297_addr_len; ++i)
    crc=crc16_update(crc, xn297_rx_addr[xn297_addr_len-i-1]);
  //process payload
  for (uint8_t i = 0; i < len+1; ++i)
    crc=crc16_update(crc, buffer[i]);
  crc=crc16_update(crc, buffer[len+1] & 0xc0, 2);
  //xorout
  if (xn297_scramble_en)
    crc ^= pgm_read_word(&xn297_crc_xorout_scrambled_enhanced[xn297_addr_len-3+len]);
  uint16_t crcxored=(buffer[len+1]<<10)|(buffer[len+2]<<2)|(buffer[len+3]>>6) ;
  if( crc == crcxored)
    return pcf_size;            // CRC  OK
  return 0;                 // CRC NOK
}
 
uint8_t XN297_IsPacketSent()
{
  return (NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_TX_DS));
}

void XN297_HoppingCalib(uint8_t num_freq)
{
  (void)num_freq;
}

void XN297_Hopping(uint8_t *hopping_frequency, uint8_t index)
{
  NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[index]);
}

void XN297_RFChannel(uint8_t number)
{ 
  NRF24L01_WriteReg(NRF24L01_05_RF_CH, number);
}

void XN297_SetPower(enum TX_Power power)
{
  NRF24L01_SetPower(power);
}

void XN297_SetFreqOffset()
{
}
