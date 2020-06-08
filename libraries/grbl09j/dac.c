
#include "grbl09j.h"

#define MCP4726_CMD_FASTWRITE           (0x00)  // Writes data to the DAC, bits 5,4 powerdown1,0
#define MCP4726_CMD_WRITEDAC            (0x40)  // Writes data to the DAC
#define MCP4726_CMD_WRITEDACEEPROM      (0x60)  // Writes data to the DAC and the EEPROM (persisting the assigned value after reset)

#define MCP4726ADDR_X      (0x60)
#define MCP4726ADDR_Y      (0x61)

// A4,A5 = PC4,PC5 = SDA,SCL = _BV(4),_BV(5) DDRC, PORTC
void i2cIni()
{
  DDRC &= ~(_BV(5)|_BV(4));
  PORTC &= ~(_BV(5)|_BV(4)); // external pullup
}

void i2cCk()
{
    // double output=delay
    // 1
    DDRC &=  ~_BV(5); 
    DDRC &=  ~_BV(5); 
    // 0
    DDRC |= _BV(5);
    DDRC |= _BV(5);
}

void i2cSt()
{
    DDRC &= ~(_BV(4)|_BV(5)); // SDA=1, SCL=1
    DDRC &= ~(_BV(4)|_BV(5)); // delay
    DDRC |= _BV(4); // SDA=0
    DDRC |= _BV(4);
    DDRC |= _BV(5); // SCL=0
}

void i2cSp()
{
    DDRC |= _BV(4)|_BV(5); // SDA=0,SCL=0
    DDRC |= _BV(4)|_BV(5);
    DDRC &= ~_BV(5); // SCL=1
    DDRC &= ~_BV(5);
    DDRC &= ~_BV(4); // SDA=1
}

void i2cWr(uint8_t b)
{
  uint8_t i=0x80;
  for(; i; i>>=1, i2cCk())
    if (i&b) DDRC &= ~_BV(4); else DDRC |= _BV(4);
  DDRC &= ~_BV(4); // release
  i2cCk(); // ACK bit
}

void i2cCk1()
{
    // double output=delay
    // 1
    PORTC |=  _BV(5);
    DDRC &=  ~_BV(5); 
    // 0
    PORTC &= ~_BV(5);
    DDRC |= _BV(5);
}

void i2cSt1()
{
    DDRC &= ~_BV(4); // SDA=1
    DDRC &= ~_BV(4);
    DDRC &= ~_BV(4);
    DDRC &= ~_BV(4);
    DDRC &= ~_BV(5); // SCL=1
    DDRC &= ~_BV(5); // delay
    DDRC &= ~_BV(5); // delay
    DDRC &= ~_BV(5); // delay
    DDRC |= _BV(4); // SDA=0
    DDRC |= _BV(4);
    DDRC |= _BV(4);
    DDRC |= _BV(4);
    DDRC |= _BV(5); // SCL=0
    DDRC |= _BV(5);
}

void i2cSp1()
{
    DDRC |= _BV(4); // SDA=0
    DDRC |= _BV(4);
    DDRC |= _BV(4);
    DDRC |= _BV(4);
    DDRC &= ~_BV(5); // SCL=1
    DDRC &= ~_BV(5); // SCL=1
    DDRC &= ~_BV(5); // SCL=1
    DDRC &= ~_BV(5); // delay
    DDRC &= ~_BV(4); // SDA=1
    DDRC &= ~_BV(4);
    DDRC &= ~_BV(4);
    DDRC &= ~_BV(4);
}

void i2cWr1(uint8_t b)
{
  uint8_t i=0x80;
  for(; i; i>>=1) {
    if (i&b) {  // 1
	PORTC|=  _BV(4);
	DDRC |=  _BV(4);
	//DDRC &= ~_BV(4);
    } else {  // 0
	PORTC&= ~_BV(4);
	DDRC |= _BV(4);
	//DDRC |= _BV(4);
    }
    i2cCk1();
  }
  DDRC &= ~_BV(4); // release
  PORTC|=  _BV(4); 
  i2cCk1(); // ACK bit
}

// very high  data transfer ~4Mbps,  130ksps (3byte)
void dac_set(uint8_t addr, int16_t output)
{
  if(output<0)output=0; else if (output > 4095)output=4095; // saturation
  i2cSt();
  i2cWr(addr<<1);// Addr[7..1], R/nW[0]=0(WRITE)
  i2cWr(((output>>8)&0xF) | MCP4726_CMD_FASTWRITE);
  i2cWr(output&0xff);
  i2cSp();
}


void dac_init()
{
	i2cIni();
	dac_set(MCP4726ADDR_X, 0);
	dac_set(MCP4726ADDR_Y, 0);
}

