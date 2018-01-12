#include <Arduino.h>

/* bits */
#define BV_MUXA 0x10
#define BV_MUXB 0x04
#define BV_MUXC 0x08
#define BV_AR16 0x40
#define BV_AR33 0x20
#define BV_REFS 0x01
#define BV_REFO 0x80

#define BV_MASKD 0xFC
#define BV_MASKB 0x01
#define BV_PORTD PORTD
#define BV_PORTB PORTB

/* modes  */
#define BV_MUX0 (0)
#define BV_MUX1 (BV_MUXA)
#define BV_MUX2 (BV_MUXB)
#define BV_MUX3 (BV_MUXB|BV_MUXA)
#define BV_MUX4 (BV_MUXC)
#define BV_MUX5 (BV_MUXC|BV_MUXA)
#define BV_MUX6 (BV_MUXC|BV_MUXB)
#define BV_MUX7 (BV_MUXC|BV_MUXB|BV_MUXA)
#define BV_MUX  (BV_MUX7)

#define BV_A17  (0)
#define BV_A33  (BV_AR16)
#define BV_A50  (BV_AR33)
#define BV_A66  (BV_AR16|BV_AR33)
#define BV_AMP  (BV_A66)

#define BV_REF777 (0)
#define BV_REF655 (BV_REFS)

#define BV_EXT  0xFE
#define BV_END  0xFF
#define BV_MUXREAD 0xFF

/* calls */
void init_mcp3201();
uint16_t read_mcp3201();
void init_mux();
uint8_t muxmode(uint8_t mode);
uint8_t muxvalues(uint8_t mode);

void uref(int s);
void refSupply(int s);
void ampSet(int s);
int ampGet(uint8_t mode);
void muxSet(int s);
int muxGet(uint8_t mode);
