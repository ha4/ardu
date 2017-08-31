#ifndef EASY_MODBUS_SLAVE_H
#define EASY_MODBUS_SLAVE_H

#include "Arduino.h"

// function definitions
int modbus_exception(uint8_t *frame, uint8_t *len, uint8_t code);
uint16_t calculateCRC(uint8_t *frame, uint8_t bufferSize);

int modbus_read(HardwareSerial *Port, unsigned char *frame, uint8_t *len,
	int eop, int *errors);

int modbus_reply(HardwareSerial *Port, unsigned char *frame, uint8_t len,
	uint8_t id, unsigned int* regs, int regsNum, int *errors);


#endif
