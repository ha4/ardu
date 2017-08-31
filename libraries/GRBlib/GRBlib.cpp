#include <GRBlib.h>

// pwm pins: 3 5 6 9 10 11

rgbled::rgbled(int pinR, int pinG, int pinB)
{
	pG=pinG; pR=pinR; pB=pinB;

	pinMode(pG, OUTPUT);
	pinMode(pR, OUTPUT);
	pinMode(pB, OUTPUT);
	r=g=b=255;
	inv = 0;
}

void rgbled::rgb(byte R, byte G, byte B)
{
	g=G; r=R; b=B;
	modx();
}

void rgbled::hsv(unsigned int H, byte S, byte V)
{
	h=H; s=S; v=V;
	hsv2rgb();
	modx();
}

void rgbled::hsl(byte H, byte S, byte L)
{
	h=H; s=S; v=L;
	hsl2rgb();
	modx();
}

/*
 * u=1-S x=(1-S)+S(1-|(H/60)mod2-1|)
 *
 *    H       R   G   B
 *
 *   0..59    V   Vx  Vu      0-R
 *  60..119   Vx  V   Vu
 * 120..179   Vu  V   Vx    120-G
 * 180..239   Vu  Vx  V
 * 240..299   Vx  Vu  V     240-B
 * 300..359   V   Vu  Vx
 *   0..59    V   Vx  Vu
 */
void rgbled::hsv2rgb()
{
	int hh, ff; unsigned long V,q,t,u; 

	while (h >= 360) h -= 360;
//	hh = h/60;  ff = h%60;
	if (h>=300) ff=h-300, hh=5;
	else if (h>=240) ff=h-240, hh=4;
	else if (h>=180) ff=h-180, hh=3;
	else if (h>=120) ff=h-120, hh=2;
	else if (h>=60) ff=h-60, hh=1;
	else ff=h, hh=0;


	V = v; V*= 255;              V/= 100;
	u = V; u*=  100 - s;         u/= 100L;
	q = V; q*= 6000 - s*ff;      q/=6000L;
	t = V; t*= 6000 - s*(60-ff); t/=6000L;
	              
	switch(hh) {
	case 0: r = V; g = t; b = u; break;
	case 1: r = q; g = V; b = u; break;
	case 2: r = u; g = V; b = t; break;
	case 3: r = u; g = q; b = V; break;
	case 4: r = t; g = u; b = V; break;
	case 5: r = V; g = u; b = q; break;
	}
}

void rgbled::hsl2rgb()
{
    uint8_t  lo, c, x, m;
    uint16_t h1, l1, H;

    l1 = v + 1;
    if (v >= 128)  l1 = 256 - l1;
    l1<<=1;
    c = (l1 * s) >> 8;

    H = h*2+h; // 0 to 1535 (actually 1530)
    H+= H;
    lo = H & 255;          // Low byte  = primary/secondary color mix
    if ((H & 256) != 0) lo = ~lo;
    h1 = lo + 1;

    x = h1 * c >> 8;
    m = v - (c >> 1);

    x+= m;
    c+= m;

    switch(H >> 8) {       // High byte = sextant of colorwheel
        case 0 : r = c; g = x; b = m; break; // R to Y
        case 1 : r = x; g = c; b = m; break; // Y to G
        case 2 : r = m; g = c; b = x; break; // G to C
        case 3 : r = m; g = x; b = c; break; // C to B
        case 4 : r = x; g = m; b = c; break; // B to M
        default: r = c; g = m; b = x; break; // M to R
    }
}


void rgbled::rgb2hsv()
{
	int m, M, d, xh;
	        
	m = (r < g)?r:g; m = (m < b)?m:b;
	M = (r > g)?r:g; M = (M > b)?M:b;

	v = 100*M/255;

	d = M - m;
	if (d < 1) { s = 0; h = 0; return; }
	if (M > 0) { s = d*100 / M; }
	else { s = 0; h = 0; return; }

	if (r >= v) xh = 60 * (g - b) / d;
	else if (g >= v) xh = 120 + 60 * (b - r) / d;
	else xh = 240 + 60*(r - g) / d;

	if (xh < 0) xh += 360; h = xh;
}

void rgbled::rgb2hsl()
{
	uint8_t m, M, d;
	int16_t xh;
	        
	m = (r < g)?r:g; m = (m < b)?m:b;
	M = (r > g)?r:g; M = (M > b)?M:b;

	d = M - m;
	v = (M+m)>>1;

	if (M == 0) { s = 0; h = 0; return; }
	if (d == 0) { s = 0; h = 0; return; }
	s = d;

	     if (r == M) xh =       120 * (g - b) / d;
	else if (g == M) xh = 240 + 120 * (b - r) / d;
	     else        xh = 480 + 120 * (r - g) / d;
	xh = xh*16/45;
	if (xh < 0) xh += 256;
	h = xh;
}

rgbpwm::rgbpwm(int pinR, int pinG, int pinB) : rgbled(pinR,pinG,pinB)
{
}

void rgbpwm::modx()
{
	if (inv) {
		analogWrite(pG, g);
		analogWrite(pR, r);
		analogWrite(pB, b);
	} else {
		analogWrite(pG, ~g);
		analogWrite(pR, ~r);
		analogWrite(pB, ~b);
	}
}

rgbeasy::rgbeasy(int pinR, int pinG, int pinB) : rgbled(pinR,pinG,pinB)
{
}

void rgbeasy::modx()
{
	if (inv) {
		digitalWrite(pG, (g>128)?1:0);
		digitalWrite(pR, (r>128)?1:0);
		digitalWrite(pB, (b>128)?1:0);
	} else {
		digitalWrite(pG, (g>128)?0:1);
		digitalWrite(pR, (r>128)?0:1);
		digitalWrite(pB, (b>128)?0:1);
	}
}

rgbpdm::rgbpdm(int pinR, int pinG, int pinB) : rgbled(pinR,pinG,pinB)
{
  pr = portOutputRegister(digitalPinToPort(pinR));
  br = digitalPinToBitMask(pinR);
  nr = ~br;
  pg = portOutputRegister(digitalPinToPort(pinG));
  bg = digitalPinToBitMask(pinG);
  ng = ~bg;
  pb = portOutputRegister(digitalPinToPort(pinB));
  bb = digitalPinToBitMask(pinB);
  nb = ~bb;
}

void rgbpdm::modx()
{
	uint16_t a;
	if (inv) {
		a = ag + g; ag=a; if(a<256) *pg &= ng; else *pg|=bg;
		a = ar + r; ar=a; if(a<256) *pr &= nr; else *pr|=br;
		a = ab + b; ab=a; if(a<256) *pb &= nb; else *pb|=bb;
	} else {
		a = ag + g; ag=a; if(a>=256) *pg &= ng; else *pg|=bg;
		a = ar + r; ar=a; if(a>=256) *pr &= nr; else *pr|=br;
		a = ab + b; ab=a; if(a>=256) *pb &= nb; else *pb|=bb;
	}
}
