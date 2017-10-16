#include "PinChangeInt.h"

enum { stb = 9, zcd = 8 };

volatile int  e,q,flashc;
volatile byte sy, disp, tmr, syncx, syncy, aa, ab, modeflash,fm;
volatile uint16_t  prev, prev90, sync, period, nextz, angle, prevg, delta, phs, zc90, eprev, znext;
volatile uint32_t  ft;
int32_t  fp,fe,fd;

// 19100*RootOf(_Z - sin(_Z) + 2*Pi*p/255 - 2*Pi)/2/Pi, p=0..255
const PROGMEM  uint16_t firea[] = {
    19004, 17485, 17059, 16758, 16518, 16312, 16132, 15970,
    15822, 15685, 15558, 15438, 15324, 15216, 15113, 15015,
    14920, 14828, 14740, 14654, 14572, 14491, 14414, 14337,
    14263, 14190, 14120, 14050, 13982, 13915, 13850, 13785,
    13722, 13660, 13600, 13539, 13480, 13422, 13364, 13308,
    13252, 13196, 13142, 13087, 13034, 12981, 12930, 12878,
    12827, 12776, 12726, 12676, 12627, 12578, 12530, 12482,
    12434, 12387, 12341, 12295, 12248, 12203, 12157, 12112,
    12067, 12022, 11979, 11934, 11891, 11847, 11804, 11761,
    11718, 11676, 11633, 11591, 11550, 11508, 11466, 11425,
    11384, 11343, 11301, 11261, 11220, 11180, 11140, 11100,
    11060, 11020, 10981, 10941, 10901, 10862, 10823, 10784,
    10745, 10707, 10667, 10628, 10590, 10552, 10513, 10474,
    10436, 10398, 10360, 10322, 10283, 10246, 10148, 10170, /*10208->10148 correction */
    10133, 10094, 10057, 10019, 9982, 9943, 9906, 9869,
    9831, 9793, 9756, 9718, 9681, 9644, 9606, 9569,
    9531, 9494, 9456, 9419, 9381, 9344, 9306, 9269,
    9231, 9194, 9156, 9119, 9081, 9043, 9006, 8967,
    8930, 8892, 8854, 8816, 8778, 8740, 8702, 8664,
    8625, 8587, 8548, 8510, 8471, 8433, 8394, 8355,
    8316, 8277, 8238, 8198, 8159, 8119, 8080, 8040,
    8000, 7960, 7920, 7879, 7839, 7798, 7757, 7716,
    7675, 7634, 7592, 7551, 7509, 7467, 7424, 7382,
    7339, 7296, 7253, 7209, 7166, 7122, 7077, 7033,
    6988, 6943, 6897, 6852, 6806, 6759, 6713, 6665,
    6618, 6570, 6521, 6473, 6423, 6374, 6324, 6273,
    6222, 6171, 6118, 6066, 6012, 5958, 5904, 5848,
    5792, 5736, 5678, 5620, 5560, 5500, 5439, 5377,
    5314, 5250, 5184, 5118, 5050, 4980, 4909, 4837,
    4762, 4687, 4608, 4528, 4445, 4360, 4272, 4180,
    4086, 3987, 3884, 3776, 3662, 3542, 3415, 3278,
    3129, 2968, 2787, 2582, 2341, 2040, 1615, 200 /* full angle correction 200*/
};

void flasher()
{
  if (flashc > 0) {
    if  (--flashc) return;
    flashc = -20 + random(18); // 150ms dark
    angle = pgm_read_word_near(firea + 255-fm);
  } else {
    if  (++flashc) return;
    flashc = 2+random(aa*10); // random 0..10aa-1
    angle = pgm_read_word_near(firea + fm);
  }
}

int16_t dfiltr(int32_t *y, int16_t x)
{
  (*y)+= (((int32_t)x*16) - *y) /16;
//  y' = y(1-a)+xa = y - ay + ax = y + ax-ay => y+= ax-ay, a'=1/a
//  ie:  a'y+=x-y/a'
  return (*y) / 16;
}

void zerocross() { 
  uint16_t zct = TCNT1; 
  uint16_t dzct = zct-prev; prev=zct;
  zc90 = zct - (dzct>>1);
  phs = zc90 - prev90; prev90=zc90;  // period
}

/* syncro generator */
ISR(TIMER1_COMPA_vect)
{
  uint16_t c = TCNT1;
  if (syncy) {
    OCR1B = c+angle;
    syncy=0;
  } else {
    interrupts(); // for true zc fire angle
    uint16_t zcnext;
    int i;

    syncy=1;
    delta = dfiltr(&fd, phs);
    zcnext = zc90+period;
    q = c - zcnext;
    e = q-eprev; eprev=q;
    i = e+q/8;
    if (i < -1) { period++; if (period > 20500) period=20500; }
    if (i > 1) { period--; if (period < 19500) period=19500; }
    sy=1;
  }
  OCR1A = c+(period>>1); // phase: 90degree
}

/* fire angle */
ISR(TIMER1_COMPB_vect)
{
  uint16_t c = TCNT1;
  if (syncx) {
    digitalWrite(stb, LOW);
    c+=200; // 100us pulse
    OCR1B = c;
    syncx=0;
  } else {
    digitalWrite(stb, HIGH);
    syncx=1;
  }
  interrupts(); // enable
  if (modeflash) flasher();
}

void setup()
{
  Serial.begin(115200);
  pinMode(stb, OUTPUT);
  digitalWrite(stb, HIGH);
  pinMode(zcd, INPUT);
  aa = analogRead(A0) >> 2;
  ab = analogRead(A0) >> 2;
  modeflash=0;
  flashc=0;
  fm=255;
  TCCR1A = 0;
  TCCR1B = 2 << CS10; // clk/8
  TCCR1C = 0;
  TCNT1  = 0;
  OCR1A=20000;
  OCR1B=20001;
  TIMSK1 &= ~((1<<OCIE1A)|(OCIE1B)); // disable OC
//  TIMSK1 |=  (1<<OCIE1A); // enable OCA
  TIMSK1 |=  (1<<OCIE1A)|(1<<OCIE1B); // enable OC
  disp=0;
  angle = 40;
  ft=0;
  period=20000;
  fe=0;
  fp=period*16L;
  fd=period*16L;
  attachPinChangeInterrupt(zcd, zerocross, CHANGE);
}

char buf[50];

void loop()
{
  if (Serial.available()) {
    switch(Serial.read()) {
      case 'a': angle = Serial.parseInt(); break;
      case 'l': angle = pgm_read_word_near(firea + Serial.parseInt()); break;
      case 'f': fm = Serial.parseInt(); break;
      case 'm': modeflash = !modeflash; break;
      case '+': period++; break;
      case '-': period--; break;
      case 'p': period  = Serial.parseInt(); break;
      case 'D': disp=!disp; break;
      case '?':
        sprintf(buf,"a:%5u\n",angle); Serial.print(buf);
        sprintf(buf,"d:%5u\n",delta); Serial.print(buf);
        sprintf(buf,"p:%5u\n",period); Serial.print(buf);
        sprintf(buf,"e:%5u\n",e); Serial.print(buf);
        sprintf(buf,"fm:%3u\n",(int)fm); Serial.print(buf);
        sprintf(buf,"disp%d flash%d\n",disp,modeflash); Serial.print(buf);
        break;
    }
  }
  analogRead(A0);
  aa = analogRead(A0) >> 2;
  if (!modeflash) if (aa != ab) { ab = aa; angle = pgm_read_word_near(firea + aa); }
  if (disp && sy) { sy=0; 
//    sprintf(buf,"%5u ", delta);  Serial.print(buf); 
  sprintf(buf,"sync d:%5u p:%5u e:%6d q:%6d a:%6u\n", delta, period, e, q, angle);  Serial.print(buf); 
 }
}

