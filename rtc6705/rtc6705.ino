#define PIN_NCS  4
#define PIN_CLK  5
#define PIN_DAT  6
#define PIN_SPI  7
#define PIN_CH1 PIN_DAT
#define PIN_CH2 PIN_NCS
#define PIN_CH3 PIN_CLK

#define RTC6705_BITDELAY delayMicroseconds(10)
#define RTC6705_SPI(v) digitalWrite(PIN_SPI, v)
#define RTC6705_CS0 digitalWrite(PIN_NCS, 0)
#define RTC6705_CS1 digitalWrite(PIN_NCS, 1)
#define RTC6705_CK0 digitalWrite(PIN_CLK, 0); RTC6705_BITDELAY
#define RTC6705_CK1 digitalWrite(PIN_CLK, 1); RTC6705_BITDELAY
#define RTC6705_INI { pinMode(PIN_SPI, OUTPUT); pinMode(PIN_CLK, OUTPUT); pinMode(PIN_DAT, OUTPUT); pinMode(PIN_NCS, OUTPUT); }
#define RTC6705_OUT pinMode(PIN_DAT, OUTPUT)
#define RTC6705_INP pinMode(PIN_DAT, INPUT)
#define RTC6705_SET(v) digitalWrite(PIN_DAT, (v)&1)
#define RTC6705_GET(v) (digitalRead(PIN_DAT)?(v):0)

uint32_t rtc6705_transfer(uint32_t v)
{
  uint32_t d,m;
  int j;

  m=1;
  d=0;

  RTC6705_CS0;
  RTC6705_OUT;
  // address part
  for(j=4;j--;) {
    RTC6705_SET(v);
    RTC6705_CK0;
    d|=RTC6705_GET(m);
    RTC6705_CK1;
    v>>=1;
    m<<=1;
  }
  // R/W part
    RTC6705_SET(v);
    RTC6705_CK0;
    d|=RTC6705_GET(m);
    RTC6705_CK1;
    m<<=1;
  // data part
  if (v&1) { // write
    v>>=1;
    for(j=20;j--;) {
      RTC6705_SET(v);
      RTC6705_CK0;
      d|=RTC6705_GET(m);
      RTC6705_CK1;
      v>>=1;
      m<<=1;
    }
  } else { // read
    RTC6705_INP;
    for(j=20;j--;) {
      RTC6705_CK0;
      d|=RTC6705_GET(m);
      RTC6705_CK1;
      m<<=1;
    }
  }

  RTC6705_CK0;
  RTC6705_CS1;
  return d;
}

void rtc6705_channel(int n)
{
  RTC6705_OUT;
  digitalWrite(PIN_CH1, n&1);
  digitalWrite(PIN_CH2, (n>>1)&1);
  digitalWrite(PIN_CH3, (n>>2)&1);
}

void put_char(char p)
{
  Serial.write(p);
}

void print_byte(uint8_t x)
{
  if (x < 0x10)Serial.print('0');
  Serial.print(x, HEX);
}

char get_char()
{
    while(Serial.available()<=0);
    return Serial.read();
}

uint32_t get_hex()
{
  uint32_t w;
  char r;
  for(w=0;;) {
    r=get_char();
    if (r>='a' && r<='f') r-='a'-'A';
    if (r>='0' && r<='9') {
      w<<=4;
      w+= r-'0';
    } else if (r>='A' && r<='F') {
      w<<=4;
      w+= r-'7';
    } else
      break;
  }
  return w;
}

void rtc6705_print(uint32_t x)
{
  Serial.print((x&0x10)?"write @":"read @");
  Serial.print(x&0xF);
  x>>=5;
  Serial.print(' ');
  Serial.print(x>>16,HEX);
  print_byte(x>>8);
  print_byte(x);
  Serial.println();
}

static uint16_t port_r[]={(uint16_t)&PORTB,(uint16_t)&PORTC,(uint16_t)&PORTD};
static uint16_t port_i[]={(uint16_t)&PINB,(uint16_t)&PINC,(uint16_t)&PIND};
static uint16_t port_d[]={(uint16_t)&DDRB,(uint16_t)&DDRC,(uint16_t)&DDRD};
int port_cmd()
{
  char p;
  int b;
  volatile uint8_t *reg, *pin, *dir;
  
  p=get_char();
  if (p >='a' || p<='z') p-='a'-'A';
  if (p < 'B' || p> 'D') return 0;
  p-='B';
  reg=(volatile uint8_t *)port_r[p];
  pin=(volatile uint8_t *)port_i[p];
  dir=(volatile uint8_t *)port_d[p];
  b=get_char()-'0';
  if (b <0 || b>7) return 0;
  b=1<<b;
  switch(get_char()) {
    case '=':
      p=get_char();
      if (p=='0') *reg &= ~b;
      else if (p=='1') *reg |= b;
      else return 0;
      break;
    case '>': *dir |= b; break;
    case '<': *dir &=~b; break;
    case '!': put_char((*reg&b)?'1':'0'); break;
    case 'd': put_char((*dir&b)?'1':'0'); break;
    default:  put_char((*pin&b)?'1':'0'); break;
  }
  return 1;
}

void serial_comm()
{
  int a;
  uint32_t d;
  switch (Serial.read()) {
    case '?':
      Serial.println("RTC6705 interface. cmd: ? r{n} w{n}{v} Pp{} c{c} s[pi] f[ixed]");
      break;
    case 'r':
      a=get_hex();
      d=rtc6705_transfer(a&0xF);
      rtc6705_print(d);
      break;
    case 'w':
      a=get_hex();
      d=get_hex();
      d=rtc6705_transfer((a&0xF)|0x10|(d<<5));
      rtc6705_print(d);
      break;
    case 'p': case 'P':
      if (port_cmd()) Serial.println("ok");
      break;
    case 'c':
      a=get_hex();
      rtc6705_channel(a);
      Serial.print("chan");
      Serial.println(a);
      break;
    case 's': RTC6705_SPI(1); Serial.println("spi mode"); break;
    case 'f': RTC6705_SPI(0); Serial.println("channel mode"); break;
  }
}

void setup()
{
  Serial.begin(115200);
  RTC6705_INI;
}

void loop()
{
  if (Serial.available()) serial_comm();
}
