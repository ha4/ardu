#ifndef OneWire_h
#define OneWire_h

#include <Arduino.h>

class OneWire
{
  private:
    uint8_t adr[8];
    char srchJ;
    uint8_t srchE;
    uint8_t pin;

  public:
    OneWire( uint8_t pin);

    uint8_t reset();
    void write_bit(uint8_t v);
    uint8_t read_bit();
    void power(bool p);

    void write(uint8_t v);
    uint8_t read();
    void write(uint8_t *d, uint8_t n);
    void read(uint8_t *d, uint8_t n);

    void start();
    uint8_t discover(uint8_t *newAddr);
    uint8_t search(uint8_t *newAddr);
    void select(uint8_t *newAddr);

    static uint8_t crc8( uint8_t *addr, uint8_t len);

};

#endif
