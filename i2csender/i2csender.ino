
#include <Wire.h>

uint8_t disp[4];

const uint8_t addr = 0x6f;

uint16_t bcd(uint8_t b) 
{
  uint16_t c;
  uint8_t n;
  for(c=0, n=0; n < 8; n++) {
   if ((c&0x0f) >= 0x05) c+=0x03;
   if ((c&0xf0) >= 0x50) c+=0x30;
   c<<=1; if(b&0x80) c++; b<<=1;
  }
  return c;
}

const uint8_t digs[] = {
  0b1111110, // 0 7E abcdefg
  0b0110000, // 1 30
  0b1101101, // 2 6D
  0b1111001, // 3 79
  0b0110011, // 4 33
  0b1011011, // 5 5B
  0b1011111, // 6 5F
  0b1110000, // 7 70
  0b1111111, // 8 7F
  0b1111011  // 9 7B
};

static uint8_t cnt = 0;

void demo()
{
    uint16_t x = bcd(++cnt);
    disp[3]=digs[x&15]; x>>=4;
    disp[2]=(x)?digs[x&15]:0; x>>=4;
    disp[1]=(x)?digs[x]:0;
    disp[0]=0;
   /* 
    static uint8_t cnt = 0;
    disp[0]=digs[cnt/10] | (disp[1]&0x80);
    disp[1]=digs[cnt%10] | (disp[2]&0x80);
    disp[2]=digs[(99-cnt)/10] | (disp[3]&0x80);
    disp[3]=digs[(99-cnt)%10] | d;
    */
}


bool every300() {
  static uint16_t t0=0;
  uint16_t t=millis();
  if (t-t0 > 300) { t0=t; return 1; }
  return 0;
}

void setup()
{
  disp[0]=0;
  disp[1]=0;
  disp[2]=0;
  disp[3]=0x80;
  Wire.begin();
 
  Serial.begin(115200);
  while (!Serial);             // Leonardo: wait for serial monitor
  Serial.println("\nI2C sender");
}

void loop()
{
   if (every300()) {
     demo();
     Wire.beginTransmission(addr);
     Wire.write(disp,4);
     if(Wire.endTransmission()) Serial.println("no transmission");
     else { 
       Serial.print(cnt);        Serial.print(' ');
       Serial.print(disp[0],HEX);Serial.print(' ');
       Serial.print(disp[1],HEX);Serial.print(' ');
       Serial.print(disp[2],HEX);Serial.print(' ');
       Serial.print(disp[3],HEX);Serial.println();
     }
   }
}

