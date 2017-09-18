#include <Arduino.h>

/* eeprom structure:
 0-1 CRC
 2   index
 3-6 data(size=4)
 7   index FF end
 */


extern const char  parc[]; // {  'S', 'P', 'I', 'D', 'T', '0', '1', 'b', 'r', 'y', 'k'/*, 'x' */};
extern void* parv[];      // {  &pidSP, &Pb, &Ti, &Td, &Ts, &R0, &T0, &beta, &Rref, &u, &kp/*, &pidPV */};

#define PARNUM sizeof(parc)
#define PARSAVE 5 /* first 5 save to eeprom */
#define PARSIZE 2

#define EEPROM_MAX 126
void crcEEprom();
void wpar();
int lpar();
void clrpar();

#define CRC_START 0xFFFF
extern uint16_t crc;
uint16_t calculateCRC16(byte x);
