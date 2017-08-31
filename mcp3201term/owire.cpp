
#if ARDUINO >= 100
#include "Arduino.h"       // for delayMicroseconds, digitalPinToBitMask, etc
#else
#include "WProgram.h"      // for delayMicroseconds
#include "pins_arduino.h"  // for digitalPinToBitMask, etc
#endif

#include "owire.h"

// Platform specific I/O definitions

#if defined(__AVR__)
#define PIN_TO_BASEREG(pin)             (portInputRegister(digitalPinToPort(pin)))
#define PIN_TO_BITMASK(pin)             (digitalPinToBitMask(pin))
#define IO_REG_TYPE uint8_t
#define IO_REG_ASM asm("r30")
#define DIRECT_READ(base, mask)         (((*(base)) & (mask)) ? 1 : 0)
#define DIRECT_MODE_INPUT(base, mask)   ((*(base+1)) &= ~(mask))
#define DIRECT_MODE_OUTPUT(base, mask)  ((*(base+1)) |= (mask))
#define DIRECT_WRITE_LOW(base, mask)    ((*(base+2)) &= ~(mask))
#define DIRECT_WRITE_HIGH(base, mask)   ((*(base+2)) |= (mask))

#elif defined(__PIC32MX__)
#include <plib.h>  // is this necessary?
#define PIN_TO_BASEREG(pin)             (portModeRegister(digitalPinToPort(pin)))
#define PIN_TO_BITMASK(pin)             (digitalPinToBitMask(pin))
#define IO_REG_TYPE uint32_t
#define IO_REG_ASM
#define DIRECT_READ(base, mask)         (((*(base+4)) & (mask)) ? 1 : 0)  //PORTX + 0x10
#define DIRECT_MODE_INPUT(base, mask)   ((*(base+2)) = (mask))            //TRISXSET + 0x08
#define DIRECT_MODE_OUTPUT(base, mask)  ((*(base+1)) = (mask))            //TRISXCLR + 0x04
#define DIRECT_WRITE_LOW(base, mask)    ((*(base+8+1)) = (mask))          //LATXCLR  + 0x24
#define DIRECT_WRITE_HIGH(base, mask)   ((*(base+8+2)) = (mask))          //LATXSET + 0x28

#else
#error "Please define I/O register types here"
#endif

OneWire::OneWire(uint8_t pin)
{
	pinMode(pin, INPUT);
	bitmask = PIN_TO_BITMASK(pin);
	baseReg = PIN_TO_BASEREG(pin);

	start_srch();
}


uint8_t OneWire::reset(void)
{
	IO_REG_TYPE mask = bitmask;
	volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;
	uint8_t r;
	uint8_t retries = 125;

	noInterrupts();
	DIRECT_MODE_INPUT(reg, mask);
	interrupts();
	// wait until the wire is high... just in case
	do {
		if (--retries == 0) return 0;
		delayMicroseconds(2);
	} while ( !DIRECT_READ(reg, mask));

	noInterrupts();
	DIRECT_WRITE_LOW(reg, mask);
	DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
	interrupts();
	delayMicroseconds(500);
	noInterrupts();
	DIRECT_MODE_INPUT(reg, mask);	// allow it to float
	delayMicroseconds(80);
	r = !DIRECT_READ(reg, mask);
	interrupts();
	delayMicroseconds(420);
	return r;
}

//
// Write a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
void OneWire::write_bit(uint8_t v)
{
	IO_REG_TYPE mask=bitmask;
	volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;

	if (v & 1) {
		noInterrupts();
		DIRECT_WRITE_LOW(reg, mask);
		DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
		delayMicroseconds(10);
		DIRECT_MODE_INPUT(reg, mask);	// drive output high
		interrupts();
		delayMicroseconds(55);
	} else {
		noInterrupts();
		DIRECT_WRITE_LOW(reg, mask);
		DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
		delayMicroseconds(65);
		DIRECT_MODE_INPUT(reg, mask);	// drive output high
		interrupts();
		delayMicroseconds(5);
	}
}

//
// Read a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
uint8_t OneWire::read_bit(void)
{
	IO_REG_TYPE mask=bitmask;
	volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;
	uint8_t r;

	noInterrupts();
	DIRECT_MODE_OUTPUT(reg, mask);
	DIRECT_WRITE_LOW(reg, mask);
	delayMicroseconds(3);
	DIRECT_MODE_INPUT(reg, mask);	// let pin float, pull up will raise
	delayMicroseconds(10);
	r = DIRECT_READ(reg, mask);
	interrupts();
	delayMicroseconds(53);
	return r;
}

void OneWire::power(bool p)
{
    noInterrupts();
    DIRECT_WRITE_HIGH(baseReg, bitmask);
    if (p)
	DIRECT_MODE_OUTPUT(baseReg, bitmask);
    else
	DIRECT_MODE_INPUT(baseReg, bitmask);
    interrupts();
}

void OneWire::write(uint8_t v) {
    uint8_t n;
    
    for (n=8; n--; v>>=1)
	write_bit(v);
}

uint8_t OneWire::read() {
    uint8_t n;
    uint8_t r = 0;
    
    for (r=0, n=8; n--; )
	r = (r>>1) | (read_bit() ? 128 : 0);

    return r;
}

void OneWire::write(uint8_t *d, uint8_t n)
{
    uint8_t i;

    for (i=0; i < n; i++)   write(d[i]);
}

void OneWire::read(uint8_t *d, uint8_t n)
{
    uint8_t i;

    for (i=0; i < n; i++)  d[i] = read();
}

void OneWire::start_srch()
{
    srchJ = -1;
    srchE = 0;
    for(int i = 8; i--;) adr[i] = 0x00;
}

uint8_t OneWire::discover(uint8_t *newAddr)
{
   int8_t lastZJ = -1;

   if (srchE) return 0;

   for(int i = 0; i < 64; i++)  {
	char msk = 1<<(i&7), ptr = i>>3;
	uint8_t  a = read_bit();
	uint8_t _a = read_bit();

	if (a && _a ) return 0;
	if (a == _a) {
                if (i==srchJ) a=1;   // if (i < srchJ) a = ((adr[ptr]&msk) != 0)?1:0;
	   else if(adr[ptr]&msk) a=1;// else           a = (i==srchJ)?1:0;
	   else lastZJ = i;          // if (a == 0) lastZJ = i;
	}

	if(a) adr[ptr] |=  msk;
	else  adr[ptr] &= ~msk;
    	write_bit(a);
   }

   srchJ = lastZJ;
   if (lastZJ == -1) srchE = 1;
   
   for (int m = 8; m--;) newAddr[m] = adr[m];
   return 1;
}

uint8_t crc8t[16]={
0x00,0x9D,0x23,0xBE,0x46,0xDB,0x65,0xF8,
0x8C,0x11,0xAF,0x32,0xCA,0x57,0xE9,0x74
};

uint8_t OneWire::crc(uint8_t *p, int n)
{
  uint8_t c=0;

  for(; n--;p ++) {
    c ^= *p;
    c = (c>>4) ^ crc8t[c&15];
    c = (c>>4) ^ crc8t[c&15];
  }

  return c;
}

