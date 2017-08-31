#include <inttypes.h>

class OneWire
{
  private:
    uint8_t adr[8];
    char srchJ;
    uint8_t srchE;
    uint8_t bitmask;
    uint8_t volatile *baseReg;
  public:
    OneWire(uint8_t p);

    uint8_t reset();
    void write_bit(uint8_t v);
    uint8_t read_bit();
    void power(bool p);

    void write(uint8_t v);
    uint8_t read();
    void write(uint8_t *d, uint8_t n);
    void read(uint8_t *d, uint8_t n);

    void start_srch();
    uint8_t discover(uint8_t *newAddr);
    uint8_t crc(uint8_t *p, int n);
};
