#ifndef _SYMAX_INTERFACE_H_
#define _SYMAX_INTERFACE_H_

#define FLAG4_VIDEO     0x80
#define FLAG4_PICTURE   0x40
#define FLAG5_HIRATE    0x80
#define FLAG5_SET1      0x40
#define FLAG6_AUTOFLIP  0x40
#define FLAG7_HEADLESS  0x80
#define STRIM_MASK      0x3F
#define STRIM_SIGN      0x20
#define SCHAN_MASK      0x7F
#define SCHAN_SIGN      0x80

struct SymaXData {
  uint8_t throttle;
  uint8_t elevator;
  uint8_t rudder;
  uint8_t aileron;
  uint8_t flags4;
  uint8_t flags5_TRelevator; 
  uint8_t flags6_TRrudder;
  uint8_t flags7_TRaileron;
};

#define SYMAX_CHAN(num) ((num<0)?0xSCHAN_SIGN|((-num)&SCHAN_MASK):num&SCHAN_MASK)
#define SYMAX_TRIM(num) ((num<0)?0xSTRIM_SIGN|((-num)&STRIM_MASK):num&STRIM_MASK)

uint16_t symax_callback();
uint8_t symax_binding();
void symax_data(struct SymaXData *x);

#endif //_SYMAX_INTERFACE_H_
