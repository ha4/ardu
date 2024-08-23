enum { clko=3, pRESET=14, CMD_nDAT=2, nWR=5, nRD=4, nCS=15,
  dB0=8,dB1,dB2,dB3,dB4,dB5,dB6=6,dB7 };
// PB0,PB1,PB2,PB2,PB3,PB4,PB5,PD6,PD7

enum { I8251_CMD=1, I8251_DAT=0};

#define SETUP_STR "19200 8,n,1"
#define SETUP_CFG (I8251_ASYNC16X|I8251_STOP1|I8251_BITS8)
#define SETUP_CMD (I8251_TXEN|I8251_RXEN|I8251_RXEN)

/* sync mode constants */
#define I8251_SYNC 0x00
#define I8251_SYN_INP 0x40 /* SYNDET pin input - external synch */
#define I8251_SYN_OUT 0x00
#define I8251_SYNC1B   0x80 /* one byte syncro symbols */
#define I8251_SYNC2B   0x00 /* two byte syncro symbol */

/* async mode constants */
#define I8251_ASYNC1X 0x01
#define I8251_ASYNC16X 0x02
#define I8251_ASYNC64X 0x03
#define I8251_STOP1  0x40
#define I8251_STOP15 0x80
#define I8251_STOP2  0xC0

/* common setup constants */
#define I8251_BITS5  0x00
#define I8251_BITS6  0x04
#define I8251_BITS7  0x08
#define I8251_BITS8  0x0C
#define I8251_PARITY 0x10
#define I8251_EVEN   0x20
#define I8251_ODD    0x00

/* commands */
#define I8251_TXEN  0x01
#define I8251_DTR   0x02
#define I8251_RXEN  0x04
#define I8251_SBRK  0x08
#define I8251_CLRERR 0x10
#define I8251_RTS   0x20
#define I8251_RESET 0x40 /* reset to SETUP state */
#define I8251_SYNCHUNT 0x80 /*  enable search for sync byte */

/* state */
#define I8251_TXRDY 0x01
#define I8251_RXRDY 0x02 /* correspond chip pin */
#define I8251_TXEMPTY 0x04 /* correspond chip pin */
#define I8251_PERR  0x08
#define I8251_OERR  0x10
#define I8251_FERR  0x20
#define I8251_SDET_BKDET 0x40 /* synchro detect/break detect, correspond chip pin */
#define I8251_DSR   0x80

void clk_init()
{
  // PD3, OC2B, D3 pin 2MHz clock
  //       mode 010 (CTC),top=OCR2A, COM2B mode=1 (toggle), src=001
  TCCR2A = (0b01 << COM2B0)| (0b10 << WGM20); // div 2
  TCCR2B = (0b0 << WGM22) | (0b001 << CS20); // clk/1 (16MHz)
  OCR2A = 4 - 1; // div 4
  OCR2B = 4 - 1;
  pinMode(clko, OUTPUT);
}

void db_put(uint8_t v)
{
  PORTD = (PORTD&0x3F)|(v&0xC0);
  PORTB = (PORTB&0xC0)|(v&0x3F);
  DDRD |= 0xC0; // output PORTD-high(6,7)
  DDRB |= 0x3F; // output PORTB-low
}

uint8_t db_get()
{
  DDRD &= 0x3F; PORTD |= 0xC0; // input&pullup PORTD-high
  DDRB &= 0xC0; PORTB |= 0x3F; // input&pullup PORTB-low
  return (PIND&0xC0)|(PINB&0x3F);
}

void chip_cs(uint8_t a) {  digitalWrite(nCS, a); }
void chip_rd(uint8_t a) {  digitalWrite(nRD, a); }
void chip_wr(uint8_t a) {  digitalWrite(nWR, a); }
void chip_adr(uint8_t a) {  digitalWrite(CMD_nDAT, a); }
void chip_reset(uint8_t res) {  digitalWrite(pRESET, res); }

uint8_t chip_read(uint8_t a)
{
  uint8_t d;

  chip_adr(a);
  chip_cs(0);
  chip_rd(0);
  d = db_get();
  chip_rd(1);
  chip_cs(1);
  return d;
}

void chip_write(uint8_t a, uint8_t d)
{
  chip_adr(a);
  chip_cs(0);
  db_put(d);
  chip_wr(0);
  chip_wr(1);
  db_get();
  chip_cs(1);
}

void chip_init()
{
  pinMode(nCS, OUTPUT);
  pinMode(nRD, OUTPUT);
  pinMode(nWR, OUTPUT);
  pinMode(CMD_nDAT, OUTPUT);
  digitalWrite(nCS, 1);
  digitalWrite(nRD, 1);
  digitalWrite(nWR, 1);
  chip_reset(0);
  pinMode(pRESET, OUTPUT);
}

void chip_softreset()
{
  chip_write(I8251_CMD, 0); /* command setup sync or 3 empty command */
  chip_write(I8251_CMD, 0);
  chip_write(I8251_CMD, 0);
  chip_write(I8251_CMD, I8251_RESET); /* actually reset */
}

void chip_setup()
{
  chip_reset(1);
  chip_reset(0);
  chip_write(I8251_CMD, SETUP_CFG);
  chip_write(I8251_CMD, SETUP_CMD);
}

void chip_tx(uint8_t b)
{
   while(!(chip_read(I8251_CMD)&I8251_TXEMPTY));
   chip_write(I8251_DAT,b);
}

uint8_t chip_rx()
{

  while(!(chip_read(I8251_CMD)&I8251_RXRDY));
  return chip_read(I8251_DAT);
}

uint8_t chip_available()
{
  return (chip_read(I8251_CMD)&I8251_RXRDY) != 0;
}

int serialTimedRead()
{
  uint32_t r;
  int c;
  
  r = millis();
  do {
    c = Serial.read();
    if (c >= 0) break;
  } while(millis() - r < 1000);

  return c;
}

uint8_t parseHex()
{
  uint8_t h;
  int c;
  h=0;
  do {
    c = serialTimedRead();
    if (c>='0' && c<='9') {
      c-='0';
      h=(h<<4)+c;
    } else if (c>='A' && c<='F') {
      c-='A';
      h=(h<<4)+c+10;
    } else if (c>='a' && c<='f') {
      c-='a';
      h=(h<<4)+c+10;
    } else c=-1;
  } while(c!=-1);
  return h;
}

void put_transfer()
{
  int b;
  while((b=serialTimedRead()) != -1) {
    chip_tx(b);
    if(b=='\n') break;
  }
  Serial.println("sent!");
}

void get_transfer()
{
  uint8_t b;
  do {
    b=chip_rx();
    Serial.print((char)b);
  } while(b!= '\n' && b!='\r');
  Serial.println("\r\nrecvd!");
}

void setup() {
  clk_init();
  chip_init();
  Serial.begin(115200);
  Serial.println("i8251A test");
}

void loop() {
  uint8_t v;
  
  if (Serial.available()>0)
  switch(v=Serial.read()) {
    case '?':
      if (serialTimedRead()=='C') {
        v=chip_read(I8251_CMD);
        Serial.print("cmd:"); 
      } else {
        v=chip_read(I8251_DAT);
        Serial.print("dat:"); 
      }
      Serial.println(v,HEX);
      break;
    case 'D': v=parseHex(); chip_write(I8251_DAT,v); Serial.print("dat="); Serial.println(v,HEX); break;
    case 'C': v=parseHex(); chip_write(I8251_CMD,v); Serial.print("cmd="); Serial.println(v,HEX); break;
    case 'R': v=parseHex(); chip_reset(v); Serial.println(v?"RESET":"unreset");break;
    case 'B': v=parseHex(); db_put(v); Serial.print("bus="); Serial.println(v,HEX); break;
    case 'b': v=db_get(); Serial.print("bus:"); Serial.println(v,HEX); break;
    case 'r': v=parseHex(); chip_rd(v); Serial.println(v?"nRD=1":"nRD=0");break;
    case 'w': v=parseHex(); chip_wr(v); Serial.println(v?"nWR=1":"nWR=0");break;
    case 'c': v=parseHex(); chip_cs(v); Serial.println(v?"nCS=1":"nCS=0");break;
    case 'a': v=parseHex(); chip_adr(v); Serial.println(v?"adr=1":"adr=0");break;
    case 'i': chip_setup(); Serial.println(SETUP_STR); break;
    case 'q': chip_softreset(); Serial.println("soft RESET"); break;
    case 'p': put_transfer(); break;
    case 'g': get_transfer(); break;
    case ' ': case '\n': case '\r': break;
    default: Serial.print(v,HEX); Serial.println("??"); break;
  }
}
