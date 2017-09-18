#define CHPX_LINES 6
#define CHPX_COLS (CHPX_LINES-1)

int chpx_s=0;
int chpx_vsync;
int disp_mode;
unsigned char chpx_display[9]; // 9x8 max
int chpx_pins[9];

void setup()
{
  chpx_pins[0]=10;
  chpx_pins[1]=9;
  chpx_pins[2]=8;
  chpx_pins[3]=7;
  chpx_pins[4]=6;
  chpx_pins[5]=5;

  
  Serial.begin(9600);
  randomSeed(analogRead(0));
  disp_mode=1;
}

int pinphase(int p, int m)
{
  int i;
  unsigned char z;

  if (p < 0) p=m-1;
  if (p >= m) { p=0; chpx_vsync=1; }

  for(z=128, i=0; i < m; i++)
    if (i==p) {
      pinMode(chpx_pins[i], OUTPUT);
      digitalWrite(chpx_pins[i], 1);
    } else {
      if (chpx_display[p] & z)
        pinMode(chpx_pins[i], OUTPUT);
      else
        pinMode(chpx_pins[i], INPUT);
      digitalWrite(chpx_pins[i], 0);
      z >>= 1;
    }
  
  return p;
}

int chpx_sync() {
  return chpx_vsync?chpx_vsync=0,1:0;
}

byte timeout(uint32_t *tm, uint32_t now, unsigned int tmout)
{
  if (now - *tm >= tmout) { 
      *tm = now;
      return 1; 
  } else
      return 0;
}

byte counter(uint16_t *cnt, uint16_t num)
{
  if (*cnt==0)
    *cnt=num;
  else
    if( --(*cnt) == 0 ) return 1;
  return 0;
}

uint32_t chpx_ref=0;

void refresh()
{
  if (!timeout(&chpx_ref, micros(), 1250)) return;
  chpx_s = 1+pinphase(chpx_s, CHPX_LINES);
}


byte bit_pixel(unsigned char *m, int x, int y, byte op)
{
  byte mask;
  while(y<0) y+=CHPX_LINES;  while(y>=CHPX_LINES) y-=CHPX_LINES;
  while(x<0) x+=CHPX_COLS;   while(x>=CHPX_COLS) x-=CHPX_COLS;
  mask = 128 >> x;
  switch(op) {
  case 0: m[y] &= ~mask; break;
  case 1: m[y] |= mask; break;
  case 2: m[y] ^= mask; break;
  case 3: mask&=m[y]; break;
  }
  return mask?1:0;
}

void clrscr()
{
  for(int y=0; y < CHPX_LINES; y++)
    for(int x=0; x < CHPX_COLS;x++)
      bit_pixel(chpx_display, x,y, 0);
}

uint32_t xwe=0;
uint16_t xyt;
int tenn[4]={1,1,1,1};

void tennis_motion()
{
  int t;
  t=tenn[0]+tenn[2];  if (t == 0 || t >= CHPX_COLS-1)  tenn[2]=-tenn[2]; tenn[0]=t;
  t=tenn[1]+tenn[3];  if (t == 0 || t >= CHPX_LINES-1) tenn[3]=-tenn[3]; tenn[1]=t;
}

void blinky()
{
  if (!timeout(&xwe, millis(), 250)) return;
  bit_pixel(chpx_display, tenn[0],tenn[1], 2);
  if (counter(&xyt, 3))
    if (++tenn[0] >= CHPX_COLS) { tenn[0]=0; if (++tenn[1] >= CHPX_LINES) tenn[1]=0; }
}

void rando_pix(unsigned char *m, int p)
{
  for(int y=0; y < CHPX_LINES; y++)
    for(int x=0; x < CHPX_COLS;x++)
      bit_pixel(m, x,y,(random(100)>p)?1:0);
}

void rando()
{
  if (!timeout(&xwe, millis(), 250)) return;
  rando_pix(chpx_display, 50);
}

int rxx=0;
void randox()
{
  if (!timeout(&xwe, millis(), 250)) return;
  if(++rxx > 100) rxx=-100;
  rando_pix(chpx_display, rxx<0?-rxx:rxx);
}

byte life_cell(unsigned char *m, int x, int y)
{
 int a,b;
 byte lex=0;
 for (a=-1;a<=1;a++) for(b=-1;b<=1;b++)
  if(!(a==0&&b==0)) lex+=bit_pixel(m,x+a,y+b,3);

 switch(lex) {
 case 2: lex=bit_pixel(m,x,y,3); break;
 case 3: lex=1; break;
 default: lex=0;
 }

 return lex;
}

byte generate_life(unsigned char *m)
{
  int x, y, s;
  byte r;
  unsigned char ne[CHPX_LINES]; // 9x8 max
  
  s=0;
  for(y=0; y < CHPX_LINES; y++)
  for(x=0; x < CHPX_COLS; x++)
  {
    r = life_cell(m,x,y);
    s+=r;
    bit_pixel(ne, x,y, r);
  }
  for(y=0; y < CHPX_LINES; y++)
    m[y]=ne[y];
  return s;
}

uint16_t  zlo=0;
void pixel_life()
{
  if (!timeout(&xwe, millis(), 250)) return;
  if (generate_life(chpx_display) == 0)
    if (counter(&zlo,4)) rando_pix(chpx_display, 80);
}

void tennis()
{
  if (!timeout(&xwe, millis(), 250)) return;
  bit_pixel(chpx_display, tenn[0], tenn[1], 2);
  tennis_motion();
  bit_pixel(chpx_display, tenn[0], tenn[1], 2);
}


void kaleido_sym(unsigned char *m, int ox, int oy)
{
  clrscr();
  for(int y=0; y < CHPX_LINES; y++)
  for(int x=0; x < CHPX_COLS; x++)
    if (bit_pixel(m, x+ox,y+oy, 3)) {
      bit_pixel(chpx_display, x, y, 2);
      bit_pixel(chpx_display, CHPX_COLS-x-1, y, 2);
      bit_pixel(chpx_display, x, CHPX_LINES-y-1, 2);
      bit_pixel(chpx_display, CHPX_COLS-x-1, CHPX_LINES-y-1, 2);
    }
}

void bitblt(unsigned char *m, int ox, int oy)
{
  clrscr();
  for(int y=0; y < CHPX_LINES; y++)
  for(int x=0; x < CHPX_COLS; x++)
    bit_pixel(chpx_display, x, y, bit_pixel(m, x+ox,y+oy, 3));
}

unsigned char life2[9]; // 9x8 max

void kaleido()
{
  if (!timeout(&xwe, millis(), 250)) return;
  rando_pix(life2, 80);
  kaleido_sym(life2, 0, 0);
}

void kailifo() // generate kaleidoscope over pixel_life
{
  if (!timeout(&xwe, millis(), 250)) return;
  if (generate_life(life2) == 0)
    if (counter(&zlo,4)) rando_pix(life2, 80);
  tennis_motion();
  kaleido_sym(life2, tenn[0], tenn[1]);
}

byte read_hex_nibble()
{
  int x;
  while ((x = Serial.read()) < 0);
  if(x >= '0' && x <= '9') return x - '0';
  if(x >= 'A' && x <= 'F') return x - 'A'+10;
  if(x >= 'a' && x <= 'f') return x - 'a'+10;
  return 0;
} 

byte read_hex()
{
    byte z = read_hex_nibble()<<4;
    return read_hex_nibble()|z;
}

void inputka()
{
  int i;
  switch(Serial.read()) {
  case 'b': // b=bitblt
    disp_mode=0; // block blinky
    for(i = 0; i < CHPX_LINES; i++)
      chpx_display[i] = read_hex();
    break;
  case 'm':
    disp_mode=Serial.parseInt();
    break;
  case 'l':
    generate_life(chpx_display);
    break;
  case 'r':
    rando_pix(chpx_display, Serial.parseInt());
    break;
  case 'c':
    clrscr();
    break;
  }
}

int mixed_state=0;
uint32_t mix_tm=0;

void mixed()
{
  if (timeout(&mix_tm, millis(), 60000)) mixed_state++;
  switch(mixed_state){
  default: mixed_state=0;
  case 0: 
  case 1: blinky(); break;
  case 2: rando(); break;
  case 3: randox(); break;
  case 4: kaleido(); break;
  case 5: pixel_life(); break;
  case 6: kailifo(); break;
  case 7: tennis(); break;
  }
}

void loop()
{
  refresh();
  if(chpx_sync())switch(disp_mode) {
  case 1: mixed(); break;
  case 2: blinky(); break;
  case 3: rando(); break;
  case 4: randox(); break;
  case 5: kaleido(); break;
  case 6: pixel_life(); break;
  case 7: kailifo(); break;
  case 8: tennis(); break;
  }
  if (Serial.available() > 0) inputka();
}

