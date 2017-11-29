#include <SPI.h>

enum { chipSelectPinADC = 9, // mcp3201 #7clk<-sck=D13, #6Dout->MISO=D12, MOSI-nc=D11, #5CS<-D9
       refSelect = 8, // mosfet
       refOut = 7, amp33 = 5, amp16 = 6, // hef4053,  refout #9=C(Z), amp33 #11=A(X), amp16 #10=B(Y)
       muxA = 4, muxB = 2, muxC = 3, // hef4051, A#11, B#10, C#9. A is lsb
};

uint16_t read_mcp3201()
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

/* 0->0.777v 1->0.666v */
void uref(int mode)
{
  digitalWrite(refSelect, mode);
  Serial.print("Uref ");
  Serial.println(mode);
}

/* 0->off, 1->on: tempr-diode, rtd(300uA), 10M-to-tc supply */
void refSupply(int mode)
{
  digitalWrite(refOut, mode);
  Serial.print("ref out ");
  Serial.println(mode);
}

/* 0->1:(16+16+33) 1->1:(16+33) 2->1:(16+16) 3->1:16 */
void ampSet(int s)
{
  digitalWrite(amp16, (s&1)?1:0);
  digitalWrite(amp33, (s&2)?1:0);
  Serial.print("amp ");
  Serial.println(s);
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
  Serial.print("mux ");
  Serial.println(s);
}

void serial_process()
{
  switch(Serial.read()) {
  case 'R': uref(1); break;
  case 'r': uref(0); break;
  case 'O': refSupply(1); break;
  case 'o': refSupply(0); break;
  case 'a': ampSet(Serial.parseInt()); break;
  case '0': muxSet(0); break;
  case '1': muxSet(1); break;
  case '2': muxSet(2); break;
  case '3': muxSet(3); break;
  case '4': muxSet(4); break;
  case '5': muxSet(5); break;
  case '6': muxSet(6); break;
  case '7': muxSet(7); break;
  }
}

void setup()
{
	Serial.begin(115200);
	SPI.begin();
	SPI.setBitOrder(MSBFIRST);
	SPI.setClockDivider(SPI_CLOCK_DIV16);
	pinMode(chipSelectPinADC, OUTPUT);
	digitalWrite(chipSelectPinADC, HIGH);
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

#define UREF 4.309545
#define UREF_PPM 6960
#define UOFS0 UREF/(1.0+30.0/6.6)
#define UOFS1 UREF/(1.0+(30.0+1.0/101.0)/5.6)
#define UOFS0_PPM -6213
#define UOFS1_PPM -15789

void loop()
{
	double mv=0;
	byte n;
	uint16_t result;

	for(mv=0, n=8; n--;) {
		result=read_mcp3201();
		mv += result;
	}
	mv/=8;
//        mv = mv * ADC_REF / 4096 + ADC_OFFS;

	Serial.print(mv);
	Serial.println("#code");
        if (Serial.available()) serial_process();
	delay(135);
}
