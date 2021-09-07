#ifndef _IFACE_XN297_H_
#define _IFACE_XN297_H_

#define XN297_UNSCRAMBLED   0
#define XN297_SCRAMBLED     1
#define XN297_CRCDIS        0
#define XN297_CRCEN         1
#define XN297_1M            0
#define XN297_250K          1
#define XN297_NRF           0
#define XN297_CC2500        1

uint8_t XN297_Configure(uint8_t crc_en, uint8_t scramble_en, uint8_t speed250k);
void XN297_SetTXAddr(const uint8_t* addr, uint8_t len);
void XN297_SetRXAddr(const uint8_t* addr, uint8_t packet_len);
void XN297_SetTxRxMode(uint8_t state);
void XN297_WritePayload(uint8_t* msg, uint8_t len);
void XN297_WriteEnhancedPayload(uint8_t* msg, uint8_t len, uint8_t noack);
uint8_t XN297_IsRX();
uint8_t XN297_ReadPayload(uint8_t* msg, uint8_t len);
uint8_t XN297_ReadEnhancedPayload(uint8_t* msg, uint8_t len);
uint8_t XN297_IsPacketSent();
void XN297_HoppingCalib(uint8_t num_freq);
void XN297_Hopping(uint8_t *hopping_frequency, uint8_t index);
void XN297_RFChannel(uint8_t number);
void XN297_SetPower(enum TX_Power power);
void XN297_SetFreqOffset();

#endif
