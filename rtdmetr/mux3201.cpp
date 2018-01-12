#include <Arduino.h>
#include "mux3201.h"

enum { chipSelectPinADC = 9, // mcp3201 #7clk<-sck=D13, #6Dout->MISO=D12, MOSI-nc=D11, #5CS<-D9
       refSelect = 8, // mosfet
       refOut = 7, amp33 = 5, amp16 = 6, // hef4053,  refout #9=C(Z), amp33 #11=A(X), amp16 #10=B(Y)
       muxA = 4, muxB = 2, muxC = 3, // hef4051, A#11, B#10, C#9. A is lsb
};


void init_mcp3201()
{
	SPI.begin();
	SPI.setBitOrder(MSBFIRST);
	SPI.setClockDivider(SPI_CLOCK_DIV16); //62.5kS/s, 1mbps.
	pinMode(chipSelectPinADC, OUTPUT);
	digitalWrite(chipSelectPinADC, HIGH);
}

uint16_t read_mcp3201() /* 30us to convert/read */
{
	uint16_t result;

	digitalWrite(chipSelectPinADC, LOW);
	result = SPI.transfer(0x00) << 8;
	result = result | SPI.transfer(0x00);
	digitalWrite(chipSelectPinADC, HIGH);
	result = result >> 1;
	result = result & 0xFFF;
  
	return result;
}

void init_mux()
{
	pinMode(refSelect, OUTPUT);
	pinMode(refOut, OUTPUT);
	pinMode(amp16, OUTPUT);
	pinMode(amp33, OUTPUT);
	pinMode(muxA, OUTPUT);
	pinMode(muxB, OUTPUT);
	pinMode(muxC, OUTPUT);

        uref(0);
        refSupply(0);
        ampSet(0);
        muxSet(4);
}

uint8_t muxmode(uint8_t mode)
{
  uint8_t b,d;

  if (mode==BV_MUXREAD) {
    b = BV_PORTB & BV_MASKB;
    d = BV_PORTD & BV_MASKD;
    return b|d;
  }
  b = mode & BV_MASKB;
  d = mode & BV_MASKD;
  BV_PORTB = (BV_PORTB & ~BV_MASKB) | b;
  BV_PORTD = (BV_PORTD & ~BV_MASKB) | d;
  return mode;
}


/* 0->0.777v 1->0.666v */
void uref(int s)
{
  digitalWrite(refSelect, s);
}

/* 0->off, 1->on: tempr-diode, rtd(300uA), 10M-to-tc supply */
void refSupply(int s)
{
  digitalWrite(refOut, s);
}

/* 0->1:(16+16+33) 1->1:(16+33) 2->1:(16+16) 3->1:16 */
void ampSet(int s)
{
  digitalWrite(amp16, (s&1)?1:0);
  digitalWrite(amp33, (s&2)?1:0);
}

int ampGet(uint8_t mode)
{
  return ((mode&BV_AR33)?2:0) | ((mode&BV_AR16)?1:0);
}

/*   A      <-R           <-mA(+)
 *   B      <-+ <-,   <-V   |
 *   B'=GND <-' <-'tc <-' <-'
 *   C  <-tc-'
 *   D  [as c]
 * channels:
 * 0:A, 1:B, 2:B(v), 3:A(mA), 4:zero, 5:Tmpr-diode, 6:C, 7:D
 */

void muxSet(int s)
{
  digitalWrite(muxA, (s&1)?1:0);
  digitalWrite(muxB, (s&2)?1:0);
  digitalWrite(muxC, (s&4)?1:0);
}

int muxGet(uint8_t mode)
{
  return ((mode&BV_MUXC)?4:0) | ((mode&BV_MUXB)?2:0) | ((mode&BV_MUXA)?1:0);
}
