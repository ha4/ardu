#include <easyModbus.h>

#define LED 9  
#define SLAVE_ID 2

#define BUFFER_SIZE 128
unsigned char frame[BUFFER_SIZE];

//3.5 symbol time: 4 for 9600, of 1.75ms for baud > 19200
#define PACKET_END_MS  4

int errorCount;



enum { pvADC=0, pvPWM, HOLDING_REGS_SIZE };

unsigned int holdingRegs[HOLDING_REGS_SIZE]; // function 3 and 16 register array


void setup()
{
  /*
     Valid modbus byte formats are:
     SERIAL_8N2: 1 start bit, 8 data bits, 2 stop bits
     SERIAL_8E1: 1 start bit, 8 data bits, 1 Even parity bit, 1 stop bit
     SERIAL_8O1: 1 start bit, 8 data bits, 1 Odd parity bit, 1 stop bit
  */
  Serial.begin(9600, SERIAL_8N2);
  errorCount = 0;
  
  pinMode(LED, OUTPUT);
}

void loop()
{
 
  uint8_t len;

  len = BUFFER_SIZE;
  if (modbus_read(&Serial, frame, &len, PACKET_END_MS, &errorCount))
      modbus_reply(&Serial, frame, len, 
	SLAVE_ID, holdingRegs, HOLDING_REGS_SIZE,
	&errorCount); // TXEnable can be here
  
  holdingRegs[pvADC] = analogRead(A0); // update data to be read by the master to adjust the PWM
  
  analogWrite(LED, holdingRegs[pvPWM]>>2); // constrain adc value from the arduino master to 255
  
}

