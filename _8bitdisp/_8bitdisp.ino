#include "usiTwiSlave.h"

const  uint8_t addr = 0x6f;
const  uint8_t div1khz = F_CPU/64/1000-1; // FOSC/64/1000-1

const uint8_t digs[] = {
	0b1111110, /* 0 7E abcdefg */
	0b0110000, /* 1 30 */
	0b1101101, /* 2 6D */
	0b1111001, /* 3 79 */
	0b0110011, /* 4 33 */
	0b1011011, /* 5 5B */
	0b1011111, /* 6 5F */
	0b1110000, /* 7 70 */
	0b1111111, /* 8 7F */
	0b1111011  /* 9 7B */
};

volatile uint16_t cntr1k=0;
static uint8_t ptr = 0;
static uint8_t disp[4];


#if defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)

#define INTVEC TIM0_COMPA_vect

// const uint8_t ledpin[] = /* h,a..g */ {7, 8, 9, 10,  3, 2, 1, 0};

static uint8_t d = 0;
static uint8_t c = 0;


void
scanner()
{
	if(d >= 8){
		d = 0;
		c = 0x80;
	}
	const uint8_t x = disp[d&3] & ((d&4)?0xF0:0x0F);
	PORTA = (PORTA & 0x70) | (x & 0x8F) ;
	DDRA = (DDRA & 0x70) | ((x | c) & 0x8F);
	PORTB = (PORTB & 0xF8) | ((x>>4) & 7);
	DDRB = (DDRB & 0xF8) | ( ((x|c)>>4) & 7 );
	++d;
	c >>= 1;
}

#else

#define INTVEC TIMER0_COMPA_vect

const uint8_t ledpin[] = /* h,a..g */ {2, 3, 4, 5,   6, 7, 8, 9};


void
scanpin(const uint8_t c, const uint8_t p, const uint8_t x, const uint8_t s)
{
	if (c == p){
		pinMode(p, OUTPUT);
		digitalWrite(p, 0);
	}
	else{
		if(x & s){
			pinMode(p, OUTPUT);
			digitalWrite(p, 1);
		}
		else
			pinMode(p, INPUT_PULLUP);
	}
}


void
scanner()
{
	static uint8_t d = 0;
	if(d >= 8) d = 0;
  
	const uint8_t x = disp[d&3] & ((d&4)?0xF0:0x0F);
	const uint8_t c = ledpin[d];  
	for(uint8_t i=0, s=0b10000000; s; ++i, s>>=1)
		scanpin(c,ledpin[i],x,s);  
	++d;
}

#endif

ISR(INTVEC)
{
	scanner();
	++cntr1k;
}


void
timer_setup()
{
	noInterrupts(); /* Set up timer 0 in mode 2 (CTC mode) */
	TCCR0B = 0;			/* clock stopped for now */
	TCNT0 = 0;
	TIFR0 |= _BV(OCF0A);	/* clear any pending interrupt */
	TIMSK0 = _BV(OCIE0A);	/* enable the timer 0 compare match interrupt */
	TCCR0A = 0b00000010;	/* no direct outputs, mode 2 wgm[2:0]=010 */
	OCR0A = div1khz;      /* why it here, not understand,  but it works only after TCCR0A set */
	TCCR0B = 0b00000011;	/* start the clock, prescaler = 64 */
	interrupts();
}


uint16_t
bcd(uint8_t b) 
{
	uint16_t c;
	uint8_t n;
  
	for(c=0, n=0; n < 8; ++n){
		if((c&0x0f) >= 0x05)
			c += 0x03;
		if((c&0xf0) >= 0x50)
			c += 0x30;
		c <<= 1;
		if(b&0x80) ++c;
		b <<= 1;
	}
	return c;
}


void
dispbcd(uint8_t n)
{
	uint16_t x = bcd(n);

	disp[3] = digs[x&15];
	x >>= 4;
	disp[2] = (x)?digs[x&15]:0;
	x >>= 4;
	disp[1] = (x)?digs[x]:0;
	disp[0] = 0;
}


void
demo()
{
	dispbcd(addr);
	/* 
	static uint8_t cnt = 0;
	disp[0]=digs[cnt/10] | (disp[1]&0x80);
	disp[1]=digs[cnt%10] | (disp[2]&0x80);
	disp[2]=digs[(99-cnt)/10] | (disp[3]&0x80);
	disp[3]=digs[(99-cnt)%10] | d;
	*/
}


bool
every300()
{
	static uint16_t t0 = 0;

	noInterrupts();
	uint16_t t = cntr1k; /*millis();*/
	interrupts();
	if (t-t0 > 300){
		t0 = t;
		return 1;
	}
	return 0;
}


void
setup()
{
	disp[0] = disp[1] = disp[2] = 0;
	disp[3] = 0x80;

	usiTwiSlaveInit(addr);
	timer_setup();
	demo();
	while(!every300());

	disp[0] = disp[1] = disp[2] = disp[3] = 0;
}


void
loop()
{
	/*
	scanner(); delay(1);
	if (every300()) demo();
	*/
	if(usiTwiDataInReceiveBuffer()){
		const uint8_t v=usiTwiReceiveByte();
		if(usiTwiDataStart())
			ptr = 0;
		if(ptr < 4)
			disp[ptr++] = v;
	}
}
