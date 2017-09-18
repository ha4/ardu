#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#define FLAG_FLIP      0x01
#define FLAG_VIDEO     0x02
#define FLAG_PICTURE   0x04
#define FLAG_HEADLESS  0x08
#define FLAG_LOW       0x10

word symax_run(int *TREAF_values);
int  symax_binding(); // 1 binding, -1 no values, 0 - ok data

#endif //_INTERFACE_H_


