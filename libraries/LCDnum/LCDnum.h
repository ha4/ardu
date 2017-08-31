#ifndef LCDnum_h
#define LCDnum_h

#include <inttypes.h>
#include "Print.h"
#include "../easySPI/easySPI.h"

// pinout:
// 1:vcc  2:strobe  3:Din  4:clk  5:Dout  6:gnd  7:base  8:gnd
// serial data bits in display
//  shift: MOSI -> right_bit0..7 -> next.. .. -> left_sign_bit0..7 -> MISO
//  4 data bytes : 
//  (firstbit)[F4]A4D4E4G4B4C4H4 F3A3D3E3G3B3C3H3 F2A2D2E2G2B2C2H2 F1A1D1E1G1B1C1H1
//
//    FADE GBCH                   FADE GBCH         FADE GBCH          FADE GBCH
//  0 1111 0110             //  E 1111 1000   //  G 1111 0010    //  v 0011 0110
//  1 0000 0110             //  F 1101 1000   //  H 1001 1110    //  w 0010 0000
//  2 0111 1100   AAAA      //  C 1111 0000   //  i 0000 0010    //  x 0010 0000
//  3 0110 1110  F    B     //  A 1101 1110   //  J 0010 0110    //  y 1010 1110
//  4 1000 1110  F    B     //  b 1011 1010   //  k 0010 0000    //  z 0010 0000
//  5 1110 1010   GGGG      //  d 0011 1110   //  m 0010 0000    //  @ 0111 1110
//  6 1111 1010  E    C     //  H 1001 1110   //  o 0011 1010    //  / 0001 1100
//  7 0100 0110  E    C     //  L 1011 0000   //  q 1100 1110    //  ( 0100 0110
//  8 1111 1110   DDDD  H   //  _ 0010 0000   //  r 0001 1000    //  ) 1101 0000
//  9 1110 1110             //deg 1100 1100   //  s 1110 1010    //  < 0010 1010
//  - 0000 1000             //  P 1101 1100   //  t 1011 1000    //  > 0011 1000
//  . 0000 0001             //  n 0001 1010   //  u 0011 0010    //  = 0010 1000


#define LCDN_A     0xDE
#define LCDN_B     0xBA
#define LCDN_C     0xF0
#define LCDN_D     0x3E
#define LCDN_E     0xF8
#define LCDN_F     0xD8
#define LCDN_COMMA 0x01
#define LCDN_MINUS 0x08
#define LCDN_0     0xF6
#define LCDN_1     0x06
#define LCDN_2     0x7C
#define LCDN_3     0x6E
#define LCDN_4     0x8E
#define LCDN_5     0xEA
#define LCDN_6     0xFA
#define LCDN_7     0x46
#define LCDN_8     0xFE
#define LCDN_9     0xEE
#define LCDN_UNDER 0x20
#define LCDN_SPACE 0x00

// Flags
#define LCDN_ON    0x01
#define LCDN_SCRL  0x02
#define LCDN_LEFT  0x04
#define LCDN_PTSET 0x08

// display maximum
#define LCDN       4
#define LCDN_MIN   0
#define LCDN_MAX   (LCDN-1)

class LCDnum : public Print, easySPI {
public:
  LCDnum(uint8_t cs, uint8_t base);
  void refresh(uint8_t flash_clk);
  void clear(), home();
  void display(), noDisplay();
  void scrollDisplay(), unScroll();
  void leftToRight(), rightToLeft();
  void autoscroll(), noAutoscroll();
//  void createChar(uint8_t, uint8_t);
  virtual size_t write(uint8_t);
  using Print::write;

private:
  uint8_t  prev();
  uint8_t  next();
  uint8_t _cs_pin;   // LOW-HIGH: latch
  uint8_t _base_pin; // oscillate in opposite phase

  uint8_t _flag;
  uint8_t _data[LCDN];
  uint8_t _curridx;
};

#endif
