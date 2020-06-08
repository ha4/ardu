
// 74hc164 pin 1&2 - DATA
#define DATAPIN 4
// 74hc164 pin 8 - CLOCK, pin 9 - CLEAR (+Vcc)
#define CLKPIN  3
#define COLPINA 5
#define COLPINB 2

#define RY0 470
#define RX0 528
#define LY0 517
#define LX0 481
#define RYM 439
#define RXM 445
#define LYM 256
#define LXM 406

// power(2,20)/MAX
#define REVM(a) (1048576L/a)
#define MAXM    (1048576L)
#define MAXS    (20-7)

#define SERIALPORT Serial


/*
L   0008 L2  0800 R2   0004 R    0400
Hu  0020 Hl  0010 Hr   2000 Hd   1000
MIN 0080 PLS 8000 Lstk 0040 Rstk 4000
X   0200 Y   0002 A    0100 B    0001
LX A3 LY A2 RX A1 RY A0
*/

uint16_t permb(uint16_t in)
{ /// 4000,8000 <-> 0020,0010
  uint16_t perm1 = (in & 0x0030) << 10;
  uint16_t perm2 = (in >> 10) & 0x0030;
  return (in & ~0xC030)|perm1|perm2;
}

/*  after permutation: ^8000 >2000 v1000 <4000
  7 0 1
 6     2
  5 4 3
*/
uint16_t hatb(uint16_t b)
{
  switch(b&0xF000) {
  case 0x8000: return 0;
  case 0xC000: return 7;
  case 0xA000: return 1;
  case 0x2000: return 2;
  case 0x1000: return 4;
  case 0x3000: return 3;
  case 0x5000: return 5;
  case 0x4000: return 6;
  default: return 8;
  }
}

uint16_t permhat(uint16_t in)
{
  return (in & 0xFFF) | (hatb(in)<<12);
}

void setup()
{
  SERIALPORT.begin(57600);
  while(!SERIALPORT);
  SERIALPORT.begin(57600);

  pinMode(DATAPIN, OUTPUT);
  pinMode(CLKPIN, OUTPUT);
  pinMode(COLPINA, INPUT_PULLUP);
  pinMode(COLPINB, INPUT_PULLUP);
}

uint16_t scanmx()
{
  uint8_t cnt, pa, pb;
  for(cnt=pa=pb=0; cnt < 8; cnt++) {
    digitalWrite(CLKPIN, 0);
    digitalWrite(DATAPIN,(cnt==0)?0:1);
    digitalWrite(CLKPIN, 1);
    delayMicroseconds(2);
    pa=(pa<<1) | (digitalRead(COLPINA)?0:1);
    pb=(pb<<1) | (digitalRead(COLPINB)?0:1);
  }
  return (pa << 8) | pb;
}

int8_t cart(int16_t a, int16_t offs, int32_t scale)
{
  int z = a-offs;
  int32_t z2 = z*scale;
  int8_t p;
  if (z2 >= MAXM) z2=MAXM-1;
  if (z2 <= -MAXM) z2=-MAXM;
  p = z2 >> MAXS;
  return p;
}

void loop()
{
  SERIALPORT.print(permhat(permb(scanmx())), HEX);
  SERIALPORT.print(' ');
  SERIALPORT.print(cart(analogRead(A0),RY0,REVM(RYM)));
  SERIALPORT.print(' ');
  SERIALPORT.print(cart(analogRead(A1),RX0,REVM(RXM)));
  SERIALPORT.print(' ');
  SERIALPORT.print(cart(analogRead(A2),LY0,REVM(LYM)));
  SERIALPORT.print(' ');
  SERIALPORT.println(cart(analogRead(A3),LX0,REVM(LXM)));
  delay(200);
}

