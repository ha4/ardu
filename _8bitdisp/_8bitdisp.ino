const uint8_t digs[] = {
  0b1111110, // 0 abcdefg
  0b0110000, // 1
  0b1101101, // 2
  0b1111001, // 3
  0b0110011, // 4
  0b1011011, // 5
  0b1011111, // 6
  0b1110000, // 7
  0b1111111, // 8
  0b1111011  // 9
};  

const uint8_t ledpin[] = /* h,a..g */ {2, 3, 4, 5,   6, 7, 8, 9};

uint8_t disp[4];
static int cnt;
  
void setup()
{
  disp[0]=digs[0]|0x80;
  disp[1]=digs[1];
  disp[2]=digs[2];
  disp[3]=digs[3];

  cnt = 0;
 
  for(uint8_t i=0; i<sizeof(ledpin); i++)
    pinMode(ledpin[i], OUTPUT);
}

bool every300() {
  static uint32_t t0=0;
  uint32_t t=millis();
  if (t-t0 > 300) { t0=t; return 1; }
  return 0;
}

void scanpin(const uint8_t c, const uint8_t p, const uint8_t x, const uint8_t s)
{
      if (c == p) { pinMode(p, OUTPUT); digitalWrite(p, 0); }
      else {
        if (x & s) { pinMode(p, OUTPUT); digitalWrite(p, 1); }
        else pinMode(p, INPUT_PULLUP);
      }
}

void scanner() {
  static uint8_t d = 0;
  if (d > 8) d = 0;
  const uint8_t x = disp[d&3] & ((d&4)?0xF0:0x0F);
  const uint8_t c = ledpin[d];  
  for(uint8_t i=0, s=0b10000000; s; i++, s>>=1)
      scanpin(c,ledpin[i],x,s);  
  d++;
}

void loop()
{
  scanner(); delay(1);
  if (every300()) {
    cnt++; if (cnt > 99) cnt = 0;
    const uint8_t d = disp[0]&0x80;
    disp[0]=digs[cnt/10] | (disp[1]&0x80);
    disp[1]=digs[cnt%10] | (disp[2]&0x80);
    disp[2]=digs[(99-cnt)/10] | (disp[3]&0x80);
    disp[3]=digs[(99-cnt)%10] | d;
  }
}
