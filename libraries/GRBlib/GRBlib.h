// pwm pins: 3 5 6 9 10 11
#ifndef __GRBledlib__
#define __GRBledlib__

#if defined(ARDUINO) && (ARDUINO >= 100)
#	include <Arduino.h>
#else
#	if !defined(IRPRONTO)
#		include <WProgram.h>
#	endif
#endif

class rgbled {
  public:
	rgbled(int pinR, int pinG, int pinB);
	void rgb(byte R, byte G, byte B);
	void hsv(unsigned int H, byte S, byte V); 
	void hsl(byte H, byte S, byte L); 
	void hsv2rgb();
	void hsl2rgb();
	void rgb2hsv();
	void rgb2hsl();
	virtual void modx() { }
	virtual void inverse(byte i=1) { }
	int  pG,pR,pB;
	byte r, g, b;
	unsigned int  h;
	byte s, v;
};

class rgbpwm : public rgbled {
  public:
	rgbpwm(int pinR, int pinG, int pinB);
	void modx();
	void inverse(byte i=1);
	byte inv;
};

class rgbeasy : public rgbled {
  public:
	rgbeasy(int pinR, int pinG, int pinB);
	void modx();
	void inverse(byte i=1);
	byte inv;
};

class rgbpdm : public rgbled {
  public:
	rgbpdm(int pinR, int pinG, int pinB);
	void modx();
	void inverse(byte i=1);
	uint16_t inv;
	byte ar, ag, ab;
	uint8_t br, bg, bb;
	uint8_t nr, ng, nb;
	volatile uint8_t *pr,*pg,*pb;
};

#endif
