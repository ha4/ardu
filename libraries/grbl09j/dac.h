
#ifndef dac4725_h
#define dac4725_h

#define MCP4726ADDR_X      (0x60)
#define MCP4726ADDR_Y      (0x61)

void i2cIni();

void i2cCk();
void i2cSt();
void i2cSp();
void i2cWr(uint8_t b);

void dac_init();
void dac_set(uint8_t addr, int16_t output);

#endif
