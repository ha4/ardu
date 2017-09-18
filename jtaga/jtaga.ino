   

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
char shell_mode = 0;

void jInitIO()
{
  JTAG_PORT = 0xFF;
  JTAG_DDR |= TCK|TMS|TDI|NRST;
  JTAG_DDR &= ~TDO;
}

unsigned jShift(uint8_t count, uint8_t d, uint8_t m)
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

void tap_idle()
{
    jShift(6, 0, 0x1F);
}

void tap_instruction(uint8_t *buf, uint8_t count)
{
  jShift(4, 0, 0x03); // TAP instruction
  for(;count > 8;count-=8, buf++)
    *buf=jShift(8, *buf, 0);
  *buf=jShift(count, *buf, 1<<(count-1));
  jShift(2, 0, 0x01); // TAP update register, go to idle
}

void tap_data(uint8_t *buf, uint8_t count)
{
  jShift(3, 0, 0x01); // TAP data register
  for(;count > 8;count-=8, buf++)
    *buf=jShift(8, *buf, 0);
  *buf=jShift(count, *buf, 1<<(count-1));
  jShift(2, 0, 0x01); // TAP update register, go to idle
}

void tap_instruction_data(uint8_t *ibuf, uint8_t icount, uint8_t *dbuf, uint8_t dcount)
{
  jShift(4, 0, 0x03);  // TAP instruction
  for(;icount > 8;icount-=8)
    jShift(8, *ibuf++, 0);
  jShift(icount, *ibuf, 1<<(icount-1));
  jShift(4, 0, 0x03);  // TAP update, go to data register
  for(;dcount > 8;dcount-=8, dbuf++)
    jShift(8, *dbuf++, 0);
  jShift(dcount, *dbuf, 1<<(dcount-1));
  jShift(2, 0, 0x01); // TAP update register, go to idle
}

void target_reset()
{
  jnReset(1);
  delay(20); // 20ms
  jnReset(0);
}


void send_str(char *buf, int n)
{
  int i;

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

void send_reply(char *buf, int n)
{
  int i;
  char c;
  
  for(i=0; i<n; i++) {
    c=buf[i];
    switch(c) {
    case '!':   
      jShift(1,0,1); 
      c=' ';      
      break;
    case '*':   
      jShift(1,0,0); 
      c=' ';      
      break;
    case '0':   
      c='0'+jShift(1,0,0);
      break;
    case '1':   
      c='0'+jShift(1,1,0);       
      break;
    case 'L':  
    case 'l': 
      c='0'+jShift(1,0,1);  
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
      jShift(8,0,0); 
      break;
    case 's':
      shell_mode=1;
    }
    buf[i]=c;
  }
  send_str(buf, n);
}

int get_str(char prompt,char *buf)
{
  int n;
  char c='\0';

  Serial.write(prompt);
  n=0;
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
  return n;  
}

void put_hex(uint8_t *buf, int count)
{
  int bys;
  bys = (count+7)/8;
  for(;bys--;) {
    Serial.print(buf[bys] >> 4, HEX);
    Serial.print(buf[bys] & 15, HEX);
  }
}

int get_hex(uint8_t *buf, char *str)
{
  int n,i;
  uint8_t nib, z;

  n=0;
  buf[0]=0;
  for(;*str;n++,str++)
    if (*str != ' ' && *str != '\t' && *str != '\b') break;
  for(;*str;) {
    nib = 255;
    if (*str>='0' && *str<='9') nib = *str-'0';
    if (*str>='A' && *str<='F') nib = *str-'A'+10;
    if (*str>='a' && *str<='f') nib = *str-'a'+10;
    str++; n++;
    if (nib >= 16) break;
    // shift long
    i=0; do {
      z=buf[i]>>4;
      buf[i]=(buf[i]<<4) | nib;
      nib=z; i++;
    } while(i < (n+1)/2);
  }
  return n;
}

void shell_exec(char *buf, int n)
{
  char *ter=buf;
  uint8_t bi[32]; // 256bits max
  uint8_t len;

  ter[n]='\0';
  switch(*ter++){
    case '0': tap_idle(); break;
    case 'r': target_reset(); break;
    case 'd': ter+=get_hex(&len, ter);
              ter+=get_hex(bi, ter);
              tap_data(bi, len);
              put_hex(bi, len);
              break;
    case 'i': ter+=get_hex(&len, ter);
              ter+=get_hex(bi, ter);
              tap_instruction(bi, len);
              put_hex(bi, len);
              break;
    case 'p': ter+=get_hex(&len,ter);
              ter+=get_hex(bi,ter);
              put_hex(bi,len);
              break;
    case 'b': shell_mode=0;
    case '\0': return;
    default: Serial.println("error"); return;
  }
}




void setup()
{
  jInitIO();
  Serial.begin(9600);
}

void loop()
{
  int n;

  n = get_str(shell_mode?'#':'>',buf);
  if (shell_mode)
  shell_exec(buf, n);
  else
  send_reply(buf, n);
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

