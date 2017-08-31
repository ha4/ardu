

//
// 1110 1110 0010 0000 0000 0101 1101 1100   >EE2005DC~ok  <3BA00477=ok
// 1000 0010 0000 0000 0100 0010 0110 0000   >82004260~ok  <06420041=ok 
//   from manual Stm32 low density: 0x06412041&0x0FFFFFFF
//  "0000"&               --4-bit Version
//  "0010000010000001"&   --16-bit Part Number (hex 2081)
//  "00001101110"&        --11-bit Manufacturer's Identity
//  "1";                  --Mandatory LSB
//
// INSTRUCTION_LENGTH of EP1C3T100 : entity is 10;
//  "BYPASS            (1111111111), "&
//  "EXTEST            (0000000000), "&
//  "SAMPLE            (0000000101), "&
//  "IDCODE            (0000000110), "&
//  "USERCODE          (0000000111), "&
//  "CLAMP             (0000001010), "&
//  "HIGHZ             (0000001011), "&
//  "CONFIG_IO         (0000001101)";






// reset=d3(pd3), tck=d4(pd4), tms=d5(pd5), tdi=d6(pd4), tdo=d7(pd7), 
#define TDO  (1<<PD7)
#define TDI  (1<<PD6)
#define TMS  (1<<PD5)
#define TCK  (1<<PD4)
#define NRST (1<<PD3)
#define JTAG_PORT PORTD
#define JTAG_DDR DDRD
#define JTAG_PIN PIND



#define BUFSIZE 128
char buf[BUFSIZE];

void jInitIO()
{
  JTAG_PORT = 0xFF;
  JTAG_DDR |= TCK|TMS|TDI|NRST;
  JTAG_DDR &= ~TDO;
}

unsigned jShift(uint8_t d, uint8_t m, uint8_t count)
{
  uint8_t x,v;

  JTAG_PORT&=~TCK;
  for (x=0, v=8;count--;v--) {
    if (d & 0x01) JTAG_PORT|=TDI; 
    else JTAG_PORT&=~TDI;
    if (m & 0x01) JTAG_PORT|=TMS; 
    else JTAG_PORT&=~TMS;
    d>>=1; 
    m>>=1;
    //    __delay_cycles(5);
    JTAG_PORT|=TCK;
    x >>= 1;
    if (JTAG_PIN & TDO) x|=0x80;
    //    __delay_cycles(5);
    JTAG_PORT&=~TCK;
  }
  for (;v--;) x >>= 1;
  return x;
}

void jnReset(uint8_t res)
{
  if (res)   JTAG_PORT&=~NRST;
  else       JTAG_PORT|=NRST;
}

void TAP(uint8_t state, uint8_t count)
{
  jShift(0, state, count);
}

void instruction(uint32_t ir, uint8_t count)
{
  union {
    uint32_t a;
    uint8_t b[4];
  } 
  x5;
  jShift(0, ir, count);
}

void instruction(uint32_t data, uint8_t count)
{
  union {
    uint32_t a;
    uint8_t b[4];
  } 
  x5;
  jShift(0, ir, count);
}

void target_reset()
{
  jnReset(1);
  delay(20); // 20ms
  jnReset(0);
}


void setup()
{
  jInitIO();
  Serial.begin(9600);
}

void loop()
{
  int i, n=0;
  char c='\0';

  // read input until buffer full or end of line
  Serial.write('>');

  while(n<BUFSIZE && c!='\n') {
    while(!Serial.available()) {
    }
    c = Serial.read();
    Serial.write(c);
    if(c == '\r') {
      c = '\n';   // normalize
      Serial.println();
    }
    if(c == '\b' && n > 0)
      n--;
    else
      buf[n++] = c;
  }

  for(i=0; i<n; i++) {
    c=buf[i];
    switch(c) {
    case '!':   
      jShift(0,1,1); 
      c=' ';      
      break;
    case '*':   
      jShift(0,0,1); 
      c=' ';      
      break;
    case '0':   
      c='0'+jShift(0,0,1);       
      break;
    case '1':   
      c='0'+jShift(1,0,1);       
      break;
    case 'L':  
    case 'l': 
      c='0'+jShift(0,1,1);  
      break;
    case 'H':  
    case 'h': 
      c='0'+jShift(1,1,1);  
      break;
    case 'R': 
      jnReset(1); 
      break;
    case 'r': 
      jnReset(0); 
      break;
    case 'w': 
      jShift(0,0,8); 
      break;
    }
    buf[i]=c;
  }

  Serial.write('=');
  for(i=0; i<n; i++) {
    char c = buf[i];
    if(c == '\n')
      Serial.println();   // "\r\n"
    else
      Serial.print(c);
  }
  Serial.flush();
}



/*
 * Count the number of devices in the JTAG chain
 */ 
// go to reset state
//	for(i=0; i<5; i++) JTAG_clock(TMS);
// go to Shift-IR
//	JTAG_clock(0);
//	JTAG_clock(TMS);
//	JTAG_clock(TMS);
//	JTAG_clock(0);
//	JTAG_clock(0);
// Send plenty of ones into the IR registers
// That makes sure all devices are in BYPASS!
//	for(i=0; i<999; i++) JTAG_clock(1);
//	JTAG_clock(1 | TMS);	// last bit needs to have TMS active, to exit shift-IR
// we are in Exit1-IR, go to Shift-DR
//	JTAG_clock(TMS);
//	JTAG_clock(TMS);
//	JTAG_clock(0);
//	JTAG_clock(0);
// Send plenty of zeros into the DR registers to flush them
//	for(i=0; i<1000; i++) JTAG_clock(0);
// now send ones until we receive one back
//	for(i=0; i<1000; i++) if(JTAG_clock(1)) break;
//	nbDevices = i;
//	printf("There are %d device(s) in the JTAG chain\n", nbDevices);


/*
 * Get the IDs of the devices in the JTAG chain
 */ 
// go to reset state (that loads IDCODE into IR of all the devices)
//	for(i=0; i<5; i++) JTAG_clock(TMS);
// go to Shift-DR
//	JTAG_clock(0);
//	JTAG_clock(TMS);
//	JTAG_clock(0);
//	JTAG_clock(0);
// and read the IDCODES
//	for(i=0; i < nbDevices; i++) {
//		printf("IDCODE for device %d is %08X\n", i+1, JTAG_read(32));
//	}

/*
 * Let's read the boundary-scan registers, and print the value on pin 99:
 */
// go to reset state
//	for(i=0; i<5; i++) JTAG_clock(TMS);
// go to Shift-IR
//	JTAG_clock(0);
//	JTAG_clock(TMS);
//	JTAG_clock(TMS);
//	JTAG_clock(0);
//	JTAG_clock(0);
// Assuming that IR is 10 bits long,
// that there is only one device in the chain,
// and that SAMPLE code = 0000000101b
//	JTAG_clock(1);
//	JTAG_clock(0);
//	JTAG_clock(1);
//	JTAG_clock(0);
//	JTAG_clock(0);
//	JTAG_clock(0);
//	JTAG_clock(0);
//	JTAG_clock(0);
//	JTAG_clock(0);
//	JTAG_clock(0 or TMS);	// last bit needs to have TMS active, to exit shift-IR
// we are in Exit1-IR, go to Shift-DR
//	JTAG_clock(TMS);
//	JTAG_clock(TMS);
//	JTAG_clock(0);
//	JTAG_clock(0);
// read the boundary-scan chain bits in an array called BSB
//	JTAG_read(BSB, 339);
//	printf("Status of pin 99 = %d\n, BSB[3]);

