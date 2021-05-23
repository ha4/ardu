#ifndef _IFACE_XN297_H_
#define _IFACE_XN297_H_

void XN297_SetTXAddr(const uint8_t* addr, uint8_t len);
void XN297_SetRXAddr(const uint8_t* addr, uint8_t len);
void XN297_Configure(uint8_t flags);
uint8_t XN297_WritePayload(uint8_t* msg, uint8_t len);
uint8_t XN297_ReadPayload(uint8_t* msg, uint8_t len);

#endif
