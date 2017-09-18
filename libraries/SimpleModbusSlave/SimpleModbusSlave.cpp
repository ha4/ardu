#include "SimpleModbusSlave.h"
#include "HardwareSerial.h"


#define BUFFER_SIZE 128

// frame[] is used to recieve and transmit packages. 
// The maximum serial ring buffer size is 128
unsigned char frame[BUFFER_SIZE];
unsigned int holdingRegsSize; // size of the register array
unsigned int* regs; // user array address
unsigned char broadcastFlag;
unsigned char slaveID;
unsigned char function;
unsigned char TxEnablePin;
unsigned int errorCount;
unsigned int T1_5; // inter character time out
unsigned int T3_5; // frame delay
HardwareSerial* ModbusPort;

// function definitions
void exceptionResponse(unsigned char exception);
unsigned int calculateCRC(unsigned char bufferSize); 
void sendPacket(unsigned char bufferSize);

void modbus_configure(HardwareSerial *SerialPort, long baud, unsigned char byteFormat,
     unsigned char _slaveID, unsigned char _TxEnablePin,
     unsigned int _holdingRegsSize, unsigned int* _regs)
{
  ModbusPort = SerialPort;
  (*ModbusPort).begin(baud, byteFormat);
  slaveID = _slaveID;
  holdingRegsSize = _holdingRegsSize; 
  regs = _regs;
  TxEnablePin = _TxEnablePin; 
  pinMode(TxEnablePin, OUTPUT);
  digitalWrite(TxEnablePin, LOW);
  errorCount = 0; // initialize errorCount
	
  // Modbus states that a baud rate higher than 19200 must use a fixed 750 us 
  // for inter character time out and 1.75 ms for a frame delay for baud rates
  // below 19200 the timing is more critical and has to be calculated.
  // E.g. 9600 baud in a 10 bit packet is 960 characters per second
  // In milliseconds this will be 960characters per 1000ms. So for 1 character
  // 1000ms/960characters is 1.04167ms per character and finally modbus states
  // an inter-character must be 1.5T or 1.5 times longer than a character. Thus
  // 1.5T = 1.04167ms * 1.5 = 1.5625ms. A frame delay is 3.5T.
	
  T1_5 = (baud > 19200) ?  750 : 15000000/baud; 
  T3_5 = (baud > 19200) ? 1750 : 35000000/baud; 
}   

unsigned int modbus_update()
{
  if (!(*ModbusPort).available()) return errorCount;

  unsigned char buffer = 0;
  unsigned char overflow = 0;
	
  while ((*ModbusPort).available()) {
    if (overflow) (*ModbusPort).read();
    else {
      if (buffer == BUFFER_SIZE) overflow = 1;
      frame[buffer] = (*ModbusPort).read();
      buffer++;
    }
    delayMicroseconds(T1_5); // inter character time out
  }
	
  if (overflow) return ++errorCount;
	
  // The minimum request packet is 8 bytes for function 3 & 16
  if (buffer > 0 && buffer < 7)  return ++errorCount; // corrupted packet

  unsigned char id = frame[0];
  broadcastFlag = (id == 0);
  if (id != slaveID && !broadcastFlag) return errorCount;

  if (calculateCRC(buffer) != 0) return ++errorCount;

  function = frame[1];
  unsigned int startingAddress = ((frame[2] << 8) | frame[3]); // combine the starting address bytes
  unsigned int no_of_registers = ((frame[4] << 8) | frame[5]); // combine the number of register bytes	
  unsigned int maxData = startingAddress + no_of_registers;
  unsigned char index;
  unsigned char address;
  unsigned int crc16;
				
  // broadcasting is not supported for function 3 
  if (!broadcastFlag && (function == 3)) {
    if (startingAddress < holdingRegsSize) {
      if (maxData <= holdingRegsSize) {// check exception 3 ILLEGAL DATA VALUE
	  unsigned char noOfBytes = no_of_registers * 2; 
 // ID, function, noOfBytes, (dataLo + dataHi)*number of registers, crcLo, crcHi
	  unsigned char responseFrameSize = 5 + noOfBytes; 
	  frame[0] = slaveID;
	  frame[1] = function;
	  frame[2] = noOfBytes;
	  address = 3; // PDU starts at the 4th byte
          unsigned int temp;

	  for (index = startingAddress; index < maxData; index++) {
		  temp = regs[index];
		  frame[address++] = temp >> 8; // split the register into 2 bytes
		  frame[address++] = temp & 0xFF;
	  }	
							
	  crc16 = calculateCRC(responseFrameSize - 2);
	  frame[responseFrameSize - 2] = crc16 & 0xFF;
	  frame[responseFrameSize - 1] = crc16 >> 8; // split crc into 2 bytes
	  sendPacket(responseFrameSize);
      }  else	
	  exceptionResponse(3); // exception 3 ILLEGAL DATA VALUE
    } else
       exceptionResponse(2); // exception 2 ILLEGAL DATA ADDRESS
  } else if (function == 16) {
	  // Check if the recieved number of bytes matches the calculated bytes 
           // minus the request bytes.
	  // id + function + (2 * address bytes) + (2 * no of register bytes) + 
           // byte count + (2 * CRC bytes) = 9 bytes
    if (frame[6] == (buffer - 9)) {
      if (startingAddress < holdingRegsSize) { // check exception 2 ILLEGAL DATA ADDRESS
        if (maxData <= holdingRegsSize) { // check exception 3 ILLEGAL DATA VALUE
	  address = 7; // start at the 8th byte in the frame
	  for (index = startingAddress; index < maxData; index++) {
		regs[index] = ((frame[address] << 8) | frame[address + 1]);
		address += 2;
	  }	
	  // only the first 6 bytes are used for CRC calculation
	  crc16 = calculateCRC(6); 
	  frame[6] = crc16 & 0xFF;
	  frame[7] = crc16 >> 8; // split crc into 2 bytes
	  // a function 16 response is an echo of the first 6 bytes from 
          // the request + 2 crc bytes
	  if (!broadcastFlag) // don't respond if it's a broadcast message
		  sendPacket(8); 
       } else	
          exceptionResponse(3); // exception 3 ILLEGAL DATA VALUE
    } else
	  exceptionResponse(2); // exception 2 ILLEGAL DATA ADDRESS
   } else  errorCount++; // corrupted packet
  } else
     exceptionResponse(1); // exception 1 ILLEGAL FUNCTION

  return errorCount++;
}	

void exceptionResponse(unsigned char exception)
{
  errorCount++; 

  if (broadcastFlag)  return;

  // exception response is always 5 bytes 
  frame[0] = slaveID;
  frame[1] = (function | 0x80); // set MSB bit high, informs the master of an exception
  frame[2] = exception;
  unsigned int crc16 = calculateCRC(3); // ID, function|0x80, exception code
  frame[3] = crc16 & 0xFF;
  frame[4] = crc16 >> 8;
  sendPacket(5); 
}

/* CRC16 Definitions */
static const unsigned short crc_table[16] = {
  0x0000, 0xcc01, 0xd801, 0x1400, 0xf001, 0x3c00, 0x2800, 0xe401,
  0xa001, 0x6c00, 0x7800, 0xb401, 0x5000, 0x9c01, 0x8801, 0x4400
};

unsigned int calculateCRC(unsigned char bufferSize) 
{ 
  unsigned int crc;

  crc = 0xFFFF;

  for(uint8_t i = 0; i < bufferSize; i++) { 
	  crc ^= frame[i];
	  crc = (crc >> 4) ^ crc_table[crc & 0x0f];
	  crc = (crc >> 4) ^ crc_table[crc & 0x0f];
  } 
  return crc; 
}

void sendPacket(unsigned char bufferSize)
{
  digitalWrite(TxEnablePin, HIGH);

  for (unsigned char i = 0; i < bufferSize; i++)
    (*ModbusPort).write(frame[i]);
		
  (*ModbusPort).flush();
	
  // allow a frame delay to indicate end of transmission
  delayMicroseconds(T3_5); 

  digitalWrite(TxEnablePin, LOW);
}