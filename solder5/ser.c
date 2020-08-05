#include <avr/io.h>
#include <avr/interrupt.h>
#include "config.h"
#include "fncs.h"

/* static */ 
/*
inline void tunedDelay0(uint16_t delay) { 
  uint8_t tmp=0;
  asm volatile("sbiw    %0, 0x01 \n\t"
    "ldi %1, 0xFF \n\t"
    "cpi %A0, 0xFF \n\t"
    "cpc %B0, %1 \n\t"
    "brne .-10 \n\t"
    : "+w" (delay), "+a" (tmp)
    : "0" (delay)
    );
}
*/

void tunedDelay(uint16_t __count)
{
	__asm__ volatile (
		"1: sbiw %0,1" "\n\t"
		"brne 1b"
		: "=w" (__count)
		: "0" (__count)
	);
}


volatile uint16_t _tx_delay2;
uint16_t _tx_delay, _rx_delay_centering, _rx_delay_intrabit, _rx_delay_stopbit;
int16_t  _rx_buffer;

#define ser_RxIntOn()  PCMSK |= SRXBIT
#define ser_RxIntOff() PCMSK &= ~SRXBIT
uint16_t sub_sat(uint16_t num, uint16_t sub) {
  if (num > sub)
    return num - sub;
  else
    return 1;
}

void ser_init(uint16_t bittime)
{
	SDDR &= ~(STXBIT|SRXBIT);
	SDDR |= STXBIT;
	SPORT |= STXBIT | SRXBIT; // out high, pullup high

	_rx_delay_centering = _rx_delay_intrabit = _rx_delay_stopbit = _tx_delay = 0;

	uint16_t bit_delay = bittime / 4;
	_tx_delay = sub_sat(bit_delay, 15 / 4);
	_tx_delay2=42;

#if GCC_VERSION > 40800
	_rx_delay_centering = sub_sat(bit_delay / 2, (4 + 4 + 75 + 17 - 23) / 4);
	_rx_delay_intrabit = sub_sat(bit_delay, 23 / 4);
//	_rx_delay_stopbit = sub_sat(bit_delay *3 / 4, (37 + 11) / 4);
	_rx_delay_stopbit = sub_sat((bittime-bit_delay) / 4, (37 + 11) / 4);
#else // Timings counted from gcc 4.3.2 output
	// WORKINGPART
	_rx_delay_centering = sub_sat(bit_delay / 2, (4 + 4 + 97 + 29 - 11) / 4);
	_rx_delay_intrabit = sub_sat(bit_delay, 11 / 4);
//	_rx_delay_stopbit = sub_sat(bit_delay *3 / 4, (44 + 17) / 4);
	_rx_delay_stopbit = sub_sat((bittime-bit_delay) / 4, (44 + 17) / 4);
#endif
	PCMSK = 0;
	GIMSK |= _BV(PCIE);
	tunedDelay(_tx_delay);
	ser_RxIntOn();
}

void ser_recv()
{
                        
#if GCC_VERSION < 40302 && 0
// Work-around for avr-gcc 4.3.0 OSX version bug
  asm volatile(
    "push r18 \n\t"
    "push r19 \n\t"
    "push r20 \n\t"
    "push r21 \n\t"
    "push r22 \n\t"
    "push r23 \n\t"
    "push r26 \n\t"
    "push r27 \n\t"
    ::);
#endif  

	uint8_t d = 0;
	if ((SPIN & SRXBIT)==0) {
		ser_RxIntOff();
		tunedDelay(_rx_delay_centering);

		for (uint8_t i=8; i > 0; --i) {
			tunedDelay(_rx_delay_intrabit);
			d >>= 1;
			if (SPIN & SRXBIT) d |= 0x80;
		}                   	
		if (_rx_buffer == -1) _rx_buffer=d;
		tunedDelay(_rx_delay_stopbit); // stopbit
		ser_RxIntOn();
	}             	

#if GCC_VERSION < 40302 && 0
  asm volatile(
    "pop r27 \n\t"
    "pop r26 \n\t"
    "pop r23 \n\t"
    "pop r22 \n\t"
    "pop r21 \n\t"
    "pop r20 \n\t"
    "pop r19 \n\t"
    "pop r18 \n\t"
    ::);
#endif
}

ISR(PCINT0_vect)
{
	ser_recv();
}

uint8_t ser_read()
{
	uint8_t oldSREG = SREG, t;
	
	cli();
	t = _rx_buffer;
	_rx_buffer=-1;
	SREG = oldSREG;
	return t;
}

uint8_t ser_available()
{
	uint8_t oldSREG = SREG, t;
	
	cli();
	t = !(_rx_buffer<0);
	SREG = oldSREG;
	return t;
}


void ser_write(uint8_t data)
{
	uint8_t oldSREG = SREG;
	cli();  // turn off interrupts for a clean txmit

	SPORT &= ~STXBIT;
	tunedDelay(_tx_delay);// + XMIT_START_ADJUSTMENT);
	for (uint8_t mask = 0x01; mask; mask <<= 1) {
		if (data & mask) SPORT |= STXBIT;
		else       SPORT &= ~STXBIT;
		tunedDelay(_tx_delay);
	}
	SPORT |= STXBIT;
	SREG = oldSREG; // turn interrupts back on
	tunedDelay(_tx_delay);
}

/*
 * higher level 
 */

void ser_print(char *s)
{
	while(*s) ser_write(*s++);
}
/*
uint16_t ser_parseInt()
{
	uint32_t t,t0;
	uint16_t d;
	uint8_t c;
	t=t0=micros();
	d=0;
	for(;t-t0 < 1000000L;) {
		t=micros();
		if (ser_available()) t0=t;
		else continue;
		c=ser_read();
		c-='0';
		if (c > 9) break;
		d*=10;
		d+=c; 
	}
	return d;
}
*/
#define BCDCORR(x)  { if ((x&0x0f) > 4) x+=3;  if (x > 0x4F) x+=0x30; }

uint16_t b2digit(uint16_t n, uint16_t b)
{
	uint8_t c='0';
//	if (b==1 || n >= b) {
		while (n >= b) c++,n-=b;
		ser_write(c);
//	}
	return n; 
}

void ser_printInt(uint16_t n)
{
//	b2digit(b2digit(b2digit(b2digit(b2digit(n,10000),1000),100),10),1);
	n=b2digit(n,10000);
	n=b2digit(n,1000);
	n=b2digit(n,100);
	n=b2digit(n,10);
	ser_write('0'+n);
}
/*
void ser_printLong(uint32_t n)
{
	uint8_t a[5];
	for(uint8_t k=5;k--;) a[k]=0;	
	for(uint8_t i=32;i--;) {
		for(uint8_t k=5;k--;) BCDCORR(a[k]);
		for(uint8_t k=5;--k;) {
			uint8_t t=a[k]<<1;
			if (a[k-1]&0x80) t++;
			a[k]=t;
		}
		a[0]<<=1; if (n&0x80000000) a[0]++;
		n<<=1;
	}

	uint8_t h;
	for(h=5;h--;) if (a[h] != 0) {
		h<<=1;
		if (a[h] > 9) h++;
		break;
	}
	if (h==0) h++;

	for(;h;h--) {
		uint8_t s=a[h>>1];
		if (h&1) s&=15; else s>>=4;
		ser_write('0'+s);
	}
}
*/
void ser_println()
{
	ser_write('\r');
	ser_write('\n');
}

