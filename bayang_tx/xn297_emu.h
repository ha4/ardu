#ifndef _IFACE_XN297EMU_H_
#define _IFACE_XN297EMU_H_


// XN297 emulation layer
enum {
	XN297_UNSCRAMBLED = 0,
	XN297_SCRAMBLED
};

// XN297 emulation layer

void XN297_SetTXAddr(const uint8_t* addr, uint8_t len);
void XN297_SetRXAddr(const uint8_t* addr, uint8_t len);
void XN297_Configure(uint8_t flags);
void XN297_SetScrambledMode(const uint8_t mode);
void XN297_WritePayload(uint8_t* msg, uint8_t len);
void XN297_WriteEnhancedPayload(uint8_t* msg, uint8_t len, uint8_t noack);
boolean XN297_ReadPayload(uint8_t* msg, uint8_t len);
uint8_t XN297_ReadEnhancedPayload(uint8_t* msg, uint8_t len);

#endif
