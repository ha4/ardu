#include "easyModbus.h"
#include "HardwareSerial.h"


// function definitions
int modbus_exception(uint8_t *frame, uint8_t *len, uint8_t code);
uint16_t calculateCRC(uint8_t *frame, uint8_t bufferSize);

int modbus_read(HardwareSerial *Port, unsigned char *frame, uint8_t *len,
	int eop, int *errors)
{
  uint8_t bp = 0;
  uint8_t overflow = 0;
  uint32_t tout;

  // inter character time out: 429 * EOPtime = 1000us * 1.5 * EOPtime / 3.5
  if (!(*Port).available()) {
    delayMicroseconds(eop*429); 
    if (!(*Port).available()) return 0;
  }
	
  while ((*Port).available()) {
    if (overflow)
      (*Port).read();
    else
      frame[bp++] = (*Port).read();
    if (bp == *len) overflow = 1;
    for(tout = millis()+eop; millis() < tout;)
	if ((*Port).available()) break;
  }
	
  if (overflow) goto err_out;
	
  // The minimum request packet is 8 bytes for function 3 & 16
  if (bp > 0 && bp < 7)  goto err_out;
  if (calculateCRC(frame, bp) != 0) goto err_out;

  *len = bp;
  return 1;
err_out:
  ++(*errors);
  return 0;
}

int modbus_reply3(unsigned char *frame, uint8_t *len, unsigned int* regs, int regsNum)
{
  uint16_t startingAddress = ((frame[2] << 8) | frame[3]);
  uint16_t no_of_registers = ((frame[4] << 8) | frame[5]);
  uint8_t *address;
  uint16_t crc16;

  if (frame[0]==0) return -1; // broadcast has no answer

  if (startingAddress >= regsNum)
    return modbus_exception(frame, len, 2); // 2 ILLEGAL DATA ADDRESS

  if (startingAddress + no_of_registers > regsNum)
    return modbus_exception(frame, len, 3); // 3 ILLEGAL DATA VALUE

 // ID, function, noOfBytes, (dataLo + dataHi)*number of registers, crcLo, crcHi
  uint8_t noOfBytes = no_of_registers * 2; 
  uint8_t responseFrameSize = 5 + noOfBytes; 

  frame[2] = noOfBytes;
  address = frame + 3; // PDU starts at the 4th byte
  for (; no_of_registers--;) {
	 *address++ = regs[startingAddress] >> 8;
	 *address++ = regs[startingAddress++] & 0xFF;
  }	
							
  crc16 = calculateCRC(frame, responseFrameSize - 2);
  frame[responseFrameSize - 2] = crc16 & 0xFF;
  frame[responseFrameSize - 1] = crc16 >> 8; // split crc into 2 bytes
  *len = responseFrameSize;
  return 0; // zero errors
}	


int modbus_reply16(unsigned char *frame, uint8_t *len, unsigned int* regs, int regsNum)
{
  uint16_t startingAddress = ((frame[2] << 8) | frame[3]);
  uint16_t no_of_registers = ((frame[4] << 8) | frame[5]);
  uint8_t *address;
  uint16_t crc16;

//  if (frame[6] == ((*len) - 9)) return -1;

  if (startingAddress >= regsNum)
    return modbus_exception(frame, len, 2); // 2 ILLEGAL DATA ADDRESS

  if (startingAddress + no_of_registers > regsNum)
    return modbus_exception(frame, len, 3); // 3 ILLEGAL DATA VALUE

  address = frame + 7; // start at the 8th byte in the frame
  for (; no_of_registers--; address+=2)
    regs[startingAddress++] = ((*address) << 8) | *(address + 1);

  crc16 = calculateCRC(frame, 6); 
  frame[6] = crc16 & 0xFF;
  frame[7] = crc16 >> 8; 
  *len = 8;

  if (frame[0]==0) return -1; // broadcast has no answer
  return 0;
}


int modbus_reply(HardwareSerial *Port, unsigned char *frame, uint8_t len,
	uint8_t id, unsigned int* regs, int regsNum, int *errors)
{
  int  rc;
  if (frame[0] != id && frame[0] != 0) return 0;

  if (frame[1]==3) 
     rc = modbus_reply3(frame, &len, regs, regsNum);
  else if (frame[1]==16) 
     rc = modbus_reply16(frame, &len, regs, regsNum);
  else if (frame[1]&=0x80) 
     rc = -1;
  else
     rc = modbus_exception(frame, &len, 1); // 1:illegal function

  if (rc < 0)  return 0;  

  (*errors) += rc;

  for (rc = 0; rc < len; rc++)
    (*Port).write(frame[rc]);
  (*Port).flush();
	
  // allow a frame delay to indicate end of transmission
  delay(3); 
}

int modbus_exception(uint8_t *frame, uint8_t *len, uint8_t code)
{
  if (frame[0] == 0)  return -1;

  // exception response is always 5 bytes 
  frame[1] |= 0x80; // informs the master of an exception
  frame[2] = code;
  unsigned int crc16 = calculateCRC(frame, 3);
  frame[3] = crc16 & 0xFF;
  frame[4] = crc16 >> 8;
  *len = 5;
  return 1; // always increment error counter
}

/* CRC16 Definitions */
static const unsigned short crc_table[16] = {
  0x0000, 0xcc01, 0xd801, 0x1400, 0xf001, 0x3c00, 0x2800, 0xe401,
  0xa001, 0x6c00, 0x7800, 0xb401, 0x5000, 0x9c01, 0x8801, 0x4400
};

uint16_t calculateCRC(uint8_t *frame, uint8_t bufferSize) 
{ 
  uint16_t crc;

  crc = 0xFFFF;

  for(uint8_t i = 0; i < bufferSize; i++) { 
	  crc ^= frame[i];
	  crc = (crc >> 4) ^ crc_table[crc & 0x0f];
	  crc = (crc >> 4) ^ crc_table[crc & 0x0f];
  } 
  return crc; 
}

