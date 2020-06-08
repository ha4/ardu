
//#include <Wire.h>

#define MCP4726_CMD_FASTWRITE           (0x00)  // Writes data to the DAC, bits 5,4 powerdown1,0
#define MCP4726_CMD_WRITEDAC            (0x40)  // Writes data to the DAC
#define MCP4726_CMD_WRITEDACEEPROM      (0x60)  // Writes data to the DAC and the EEPROM (persisting the assigned value after reset)

#define MCP4726ADDR_X      (0x60)
#define MCP4726ADDR_Y      (0x61)

/*
void setVoltage(uint8_t addr, uint16_t output, bool writeEEPROM )
{
  Wire.beginTransmission(addr);
  Wire.write(writeEEPROM?MCP4726_CMD_WRITEDACEEPROM:MCP4726_CMD_WRITEDAC);
  Wire.write(output / 16);                   // Upper data bits          (D11.D10.D9.D8.D7.D6.D5.D4)
  Wire.write((output % 16) << 4);            // Lower data bits          (D3.D2.D1.D0.x.x.x.x)
  Wire.endTransmission();
}
*/


// A4,A5 = PC4,PC5 = SDA,SCL = _BV(4),_BV(5) DDRC, PORTC
void i2cIni()
{
  DDRC &= ~(_BV(5)|_BV(4));
  PORTC &= ~(_BV(5)|_BV(4)); // external pullup
}

void i2cCk()
{
    DDRC &=  ~_BV(5);
    DDRC &=  ~_BV(5); // delay
    DDRC |= _BV(5);
    DDRC |= _BV(5); // delay
}

void i2cSt()
{
    DDRC &= ~_BV(4); // SDA=1
    DDRC &= ~_BV(4);
    DDRC &= ~_BV(5); // SCL=1
    DDRC &= ~_BV(5); // delay
    DDRC |= _BV(4); // SDA=0
    DDRC |= _BV(4);
    DDRC |= _BV(5); // SCL=0
}

void i2cSp()
{
    DDRC |= _BV(4); // SDA=0
    DDRC |= _BV(4);
    DDRC &= ~_BV(5); // SCL=1
    DDRC &= ~_BV(5); // delay
    DDRC &= ~_BV(4); // SDA=1
    DDRC &= ~_BV(4);
}

void i2cWr(uint8_t b)
{
  uint8_t i=0x80;
  for(; i; i>>=1) {
    if (i&b) DDRC &= ~_BV(4); else DDRC |= _BV(4);
    i2cCk();
  }
  DDRC &= ~_BV(4); i2cCk(); // ACK bit
}

// very high  data transfer ~4Mbps,  130ksps (3byte)
void dac_set(uint8_t addr, uint16_t output)
{
  i2cSt();
  i2cWr(addr<<1);// R/nW=0, WRITE
  i2cWr(highByte(output) | MCP4726_CMD_FASTWRITE);
  i2cWr(lowByte(output));
  i2cSp();
}


void setup()
{
  Serial.begin (115200);
  while (!Serial) {}

  Serial.println ("ok");
  
//  Wire.begin();
//  TWBR = ((F_CPU / 400000L) - 16) / 2; // Set I2C frequency to 400kHz
   i2cIni();
}

void loop()
{
  if (Serial.available()) switch(Serial.read()) {
//    case 'x': case 'X': setVoltage(MCP4726ADDR_X, Serial.parseInt(), false); break;
//    case 'y': case 'Y': setVoltage(MCP4726ADDR_Y, Serial.parseInt(), false); break;
    case 'x': case 'X': dac_set(MCP4726ADDR_X, Serial.parseInt()); break;
    case 'y': case 'Y': dac_set(MCP4726ADDR_Y, Serial.parseInt()); break;
  }
}

