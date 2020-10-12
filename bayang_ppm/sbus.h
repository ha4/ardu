#ifndef __SBUS_H
#define __SBUS_H

#define SBUS_PERIOD 14000L
#define SBUS_HIGHSPEED_PERIOD    7000L
#define SBUS_FLAG_SIGNAL_LOSS       (1 << 2)
#define SBUS_FLAG_FAILSAFE_ACTIVE   (1 << 3)
#define SBUS_FLAG_CHAN17            (1 << 1)
#define SBUS_FLAG_CHAN16            (1 << 0)

typedef struct sbusChannels_s {
    // 176 bits of data (11 bits per channel * 16 channels) = 22 bytes.
    unsigned int chan0 : 11;
    unsigned int chan1 : 11;
    unsigned int chan2 : 11;
    unsigned int chan3 : 11;
    unsigned int chan4 : 11;
    unsigned int chan5 : 11;
    unsigned int chan6 : 11;
    unsigned int chan7 : 11;
    unsigned int chan8 : 11;
    unsigned int chan9 : 11;
    unsigned int chan10 : 11;
    unsigned int chan11 : 11;
    unsigned int chan12 : 11;
    unsigned int chan13 : 11;
    unsigned int chan14 : 11;
    unsigned int chan15 : 11;
    uint8_t flags;
} __attribute__((__packed__)) sbusChannels_t;

void sbus_start(); 
void sbus_send();

#endif
