#include <Arduino.h>

#include "config.h"

#include "ps2device.h"


#ifndef PS2_CLK_PORT
#define PS2_CLK_PORT    PORTB
#define PS2_CLK_DDR     DDRB
#define PS2_CLK_IN      PINB
#define PS2_CLK_PIN	0 /* D8 */
#endif

#ifndef PS2_DATA_PORT
#define PS2_DATA_PORT   PORTB
#define PS2_DATA_DDR    DDRB
#define PS2_DATA_IN     PINB
#define PS2_DATA_PIN    2 /* D10 */
#endif




static void clkHi(void)
{
  PS2_CLK_DDR  &= ~_BV(PS2_CLK_PIN);
  PS2_CLK_PORT |= _BV(PS2_CLK_PIN);
}

static void clkLo(void)
{
  PS2_CLK_PORT &= ~_BV(PS2_CLK_PIN);
  PS2_CLK_DDR  |= _BV(PS2_CLK_PIN);
}

static void dataHi(void)
{
  PS2_DATA_DDR  &= ~_BV(PS2_DATA_PIN);
  PS2_DATA_PORT |= _BV(PS2_DATA_PIN);
}

static void dataLo(void)
{
  PS2_DATA_PORT &= ~_BV(PS2_DATA_PIN);
  PS2_DATA_DDR  |= _BV(PS2_DATA_PIN);
}

#define clkPin() (PS2_CLK_IN & _BV(PS2_CLK_PIN))
#define dataPin() (PS2_DATA_IN & _BV(PS2_DATA_PIN))

static inline uint16_t wait_clkLo(uint16_t us)
{
    while (clkPin()  && us) { asm(""); delayMicroseconds(1); us--; }
    return us;
}
static inline uint16_t wait_clkHi(uint16_t us)
{
    while (!clkPin() && us) { asm(""); delayMicroseconds(1); us--; }
    return us;
}
static inline uint16_t wait_dataLo(uint16_t us)
{
    while (dataPin() && us)  { asm(""); delayMicroseconds(1); us--; }
    return us;
}
static inline uint16_t wait_dataHi(uint16_t us)
{
    while (!dataPin() && us)  { asm(""); delayMicroseconds(1); us--; }
    return us;
}
static inline uint16_t wait_available(uint16_t us)
{
    while (clkPin() && dataPin() && us) { asm(""); delayMicroseconds(1); us--; }
    return us;
}


void ps2_init(void)
{
  clkHi();
  dataHi();
}

bool  ps2_txSet(unsigned char d)
{
	unsigned char parity;
	
	parity = 0; dataLo(); delayMicroseconds(25);
	if (!clkPin()) goto ERRT; clkLo(); delayMicroseconds(25);

	clkHi(); if (d&1) {parity++; dataHi();} else dataLo(); d>>=1; delayMicroseconds(25);
	if (!clkPin()) goto ERRT; clkLo(); delayMicroseconds(25);
	clkHi(); if (d&1) {parity++; dataHi();} else dataLo(); d>>=1; delayMicroseconds(25);
	if (!clkPin()) goto ERRT; clkLo(); delayMicroseconds(25);
	clkHi(); if (d&1) {parity++; dataHi();} else dataLo(); d>>=1; delayMicroseconds(25);
	if (!clkPin()) goto ERRT; clkLo(); delayMicroseconds(25);
	clkHi(); if (d&1) {parity++; dataHi();} else dataLo(); d>>=1; delayMicroseconds(25);
	if (!clkPin()) goto ERRT; clkLo(); delayMicroseconds(25);
	clkHi(); if (d&1) {parity++; dataHi();} else dataLo(); d>>=1; delayMicroseconds(25);
	if (!clkPin()) goto ERRT; clkLo(); delayMicroseconds(25);
	clkHi(); if (d&1) {parity++; dataHi();} else dataLo(); d>>=1; delayMicroseconds(25);
	if (!clkPin()) goto ERRT; clkLo(); delayMicroseconds(25);
	clkHi(); if (d&1) {parity++; dataHi();} else dataLo(); d>>=1; delayMicroseconds(25);
	if (!clkPin()) goto ERRT; clkLo(); delayMicroseconds(25);
	clkHi(); if (d&1) {parity++; dataHi();} else dataLo(); d>>=1; delayMicroseconds(25);
	if (!clkPin()) goto ERRT; clkLo(); delayMicroseconds(25);

        clkHi(); if (parity & 1) dataLo(); else dataHi(); delayMicroseconds(25);
	if (!clkPin()) goto ERRT; clkLo(); delayMicroseconds(25);

        clkHi(); dataHi(); delayMicroseconds(25);
	if (!clkPin()) goto ERRT; clkLo(); delayMicroseconds(25);

        clkHi();

	return 1;
ERRT:
        dataHi();
	return 0;
}

bool ps2_rxGet(unsigned char * r)
{
	unsigned char data;
	unsigned char parity, err=0;
	if (!wait_clkHi(100)) goto ERRX;data = 0;  if (dataPin()) goto ERRX;    parity=0;   clkLo(); delayMicroseconds(25); // start bit
	clkHi(); delayMicroseconds(25);	data >>=1; if (dataPin()) { data|=0x80; parity++; } clkLo(); delayMicroseconds(25); // 0
	clkHi(); delayMicroseconds(25); data >>=1; if (dataPin()) { data|=0x80; parity++; } clkLo(); delayMicroseconds(25);
	clkHi(); delayMicroseconds(25); data >>=1; if (dataPin()) { data|=0x80; parity++; } clkLo(); delayMicroseconds(25);
	clkHi(); delayMicroseconds(25); data >>=1; if (dataPin()) { data|=0x80; parity++; } clkLo(); delayMicroseconds(25);
	clkHi(); delayMicroseconds(25); data >>=1; if (dataPin()) { data|=0x80; parity++; } clkLo(); delayMicroseconds(25);
	clkHi(); delayMicroseconds(25); data >>=1; if (dataPin()) { data|=0x80; parity++; } clkLo(); delayMicroseconds(25);
	clkHi(); delayMicroseconds(25); data >>=1; if (dataPin()) { data|=0x80; parity++; } clkLo(); delayMicroseconds(25);
	clkHi(); delayMicroseconds(25); data >>=1; if (dataPin()) { data|=0x80; parity++; } clkLo(); delayMicroseconds(25); // 7
	clkHi(); delayMicroseconds(25); if (dataPin()) { parity++;} if (!(parity&1)) err++; clkLo(); delayMicroseconds(25); // parity
	clkHi(); delayMicroseconds(25); if (!dataPin()) err++; // stopbit
	if (!err) dataLo();  clkLo(); delayMicroseconds(25); // ack
	clkHi(); delayMicroseconds(25); dataHi(); // ACK
	if (err) goto ERRX;
	*r = data;
	return 1;
ERRX:
	return 0;
}

bool ps2_rxAvailable(void)
{
	clkHi(); dataHi();
	if (clkPin() && dataPin()) return 0;
	return 1;
}

bool ps2_txClear(void)
{
	return 	!ps2_rxAvailable();
}

