#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#define FLAG_FLIP      0x01
#define FLAG_VIDEO     0x02
#define FLAG_PICTURE   0x04
#define FLAG_HEADLESS  0x08

word symax_callback();
byte symax_binding();
void symax_TREAF(byte _throttle, byte _rudder, byte _elevator, byte _aileron, byte _flags);

#endif //_INTERFACE_H_
