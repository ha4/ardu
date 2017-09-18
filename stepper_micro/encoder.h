#ifndef _ENC_

#define ENC_NULL    0
#define ENC_CCW     1
#define ENC_CW      2
#define ENC_RELEASE 4
#define ENC_CLICK   5
#define ENC_LONG    6
#define ENC_DOUBLE  7

#define ENC_DENOISE 5 /*ms*/
#define ENC_TDENOISE 20 /*ms*/
#define ENC_TCLICK  600 /*ms*/
#define ENC_TLONG   1000 /*ms*/
#endif

void enc_init();
int inputRot();
int inputBttn();
