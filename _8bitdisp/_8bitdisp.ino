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

const uint8_t ledpin[] = /* a..g */ {  3, 4, 5, 6, 7, 8, 9 };

uint8_t disp[4];
static int cnt;
  
void setup()
{
  disp[0]=digs[0];
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

void loop()
{
  for(uint8_t i=0, s=0b1000000; s; i++, s>>=1)
    digitalWrite(ledpin[i], (disp[0] & s)?1:0);
  if (every300()) {
    cnt++; if (cnt > 9) cnt = 0;
    disp[0]=digs[cnt];
    disp[1]=digs[9-cnt];
    disp[2]=digs[(cnt+1)%10];
    disp[3]=digs[(cnt+2)%10];
  }
}
