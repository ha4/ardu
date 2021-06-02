#ifndef _SYMAX_DATA_H_
#define _SYMAX_DATA_H_

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
  int8_t elevator; // pitch
  int8_t rudder;   // yaw
  int8_t aileron;  // roll
  int8_t trims[3]; // ERA +/-31
  uint8_t flags4;
  uint8_t flags5; 
  uint8_t flags6;
  uint8_t flags7;
};

#define SYMAX_CHAN(num) ((num<0)?SCHAN_SIGN|((-num)&SCHAN_MASK):num&SCHAN_MASK)
#define SYMAX_TRIM(num) ((num<0)?STRIM_SIGN|((-num)&STRIM_MASK):num&STRIM_MASK)

#define CHAN_SYMAX(num) ((num&SCHAN_SIGN)?-(num&SCHAN_MASK):(num))
#define TRIM_SYMAX(num) ((num&STRIM_SIGN)?-(num&STRIM_MASK):(num&STRIM_MASK))

#endif

#ifndef _SYMAX_INTERFACE_H_
#define _SYMAX_INTERFACE_H_

uint16_t symax_init();
uint16_t symax_callback();
uint16_t symax_bind();

void symax_tx_id(uint32_t id); // default 0x7F7FC0D7ul;
void symax_data(struct SymaXData *x);
int8_t symax_state();

#endif //_SYMAX_INTERFACE_H_
