#define NOTE_C_7  2093
#define NOTE_Cs7  2217
#define NOTE_D_7  2349
#define NOTE_Ds7  2489
#define NOTE_E_7  2637
#define NOTE_F_7  2794
#define NOTE_Fs7  2960
#define NOTE_G_7  3136
#define NOTE_Gs7  3322
#define NOTE_A_7  3520
#define NOTE_As7  3729
#define NOTE_B_7  3951

// notes in the melody:
enum {NN=0,
  Cs=1,  Ds,
  E,   Fs,  Gs,  A,   B,Bs, Cs2, Ds2,
  E_2, Fs2, Gs2, A_2, B_2,  Cs3, Ds3,
  E_3 };
int notes[] = { 0,
                                                                          NOTE_Cs7/8, NOTE_Ds7/8,
  NOTE_E_7/8, NOTE_Fs7/8, NOTE_Gs7/8, NOTE_A_7/8, NOTE_B_7/8, NOTE_C_7/4, NOTE_Cs7/4, NOTE_Ds7/4,
  NOTE_E_7/4, NOTE_Fs7/4, NOTE_Gs7/4, NOTE_A_7/4, NOTE_B_7/4,             NOTE_Cs7/2, NOTE_Ds7/2,
  NOTE_E_7/2 
};

const byte melody[] PROGMEM = { // in 1/16
// 1-4
  E_2,0,     E_2,0,     Cs2,0,    Cs2,0,
  Gs,0,      Gs,0,      E,0,0,0,
  Gs,0,      Gs,0,      E,0,      E,0,
  Cs,0,0,0,             Cs,0,0,0,
// 5-8
  E,0,       E,0,       Gs,0,     Gs,0,
  Cs2,0,     Cs2,0,     E_2,0,0,0,
  Fs2,0,0,0,            B_2,0,0,0,         B_2, A_2, Gs2, Fs2,
  Gs2,0,0,0,            Gs2,0,0,0,
// 9-12
  E_2,0,     E_2,0,     Cs2,0,    E_2,Cs2,
  B,0,       B,0,       Gs,0,0,0,
  Cs2,0,0,0,            Cs2,0,0,0,         Cs2, B, A, Gs,
  Fs,0,0,0,             Fs,0,0,0,
// 13-16
  E_2,0,0,0,            Gs2,0,0, E_2,
  Cs2,0,     Cs2,0,     Cs2,0,0,0,
  Gs,0,0,0,             Gs, Bs, Ds2, Bs,
  Gs,0,0,0,             Gs,0,0,0,
// 17-20
  E_3,0,     E_3,0,     Cs3,0,     Cs3,0,
  Gs2,0,     Gs2,0,     E_2,0,0,0,
  Gs2,0,     Gs2,0,     E_2,0,     E_2,0,
  Cs2,0,0,0,            Cs2,0,0,0,
// 21-24
  E_2,0,     E_2,0,     Gs2,0,     Gs2,0,
  Cs3,0,     Cs3,0,     E_3,0,0,0,
  Fs2,0,0,0,            B_2,0,0,0,         B_2, A_2, Gs2, Fs2,
  Gs2,0,0,0,            Gs2,0,0,0,
// 25-28
  E_3,0,     E_3,0,     Cs3,0,     E_3,Cs3,
  B_2,0,     B_2,0,     Gs2,0,0,0,
  Cs3,0,0,0,            Cs3,0,0,0,         Cs3, B_2, A_2, Gs2,
  Fs2,0,0,0,            Fs2,0,0,0,
// 29-32
  E_2,0,0,0,            Gs2,0,0,   E_2,
  Cs2,0,     Cs2,0,     Cs2,0,0,0,
  Gs,0,0,0,             Gs, Bs, Ds2, Bs,
  Cs2,0,0,0,            Cs2,0,0,0,
// 33-36
  Cs3,0,     Ds3,0,     E_3,0,0,   Ds3,    E_3, Ds3, Cs3, /*B_2,*/ A_2, /*Gs2,*/
  Fs2,0,     Fs2,0,     Fs2,0,0,0,
  E_2,0,0,0,            Gs2,0,0,0,
  Gs2,0,0,     Gs2,     E_2,0,0,   E_2,
// 37-40
  Cs2,0,0,0,            Cs2,0,0,0,
  Cs2,0,0,0,            E_2,0,0,0,
//  E_2,0,0,0,            E_2,0,0,0,
//  E_2,0,0,0,            E_2,0,0,0,
};
#define M_SZ (sizeof(melody)/sizeof(byte))

//  0  0  0  1  2  3  5  7  9 11 14 16 19 22 25 28
// 31 35 38 41 44 47 49 52 54 56 58 60 61 62 63 63
// 64 63 63 62 61 60 58 56 54 52 49 47 44 41 38 35
// 32 28 25 22 19 16 14 11  9  7  5  3  2  1  0  0

void setup() {
  Serial.begin(9600);
  pinMode(8,OUTPUT);

  // iterate over the notes of the melody:
//  for (int thisNote = 0; thisNote < M_SZ; thisNote++) {
//    int freq = notes[pgm_read_byte_near(melody+thisNote)];
    
//    if(freq!=0) tone(8, freq, 100); // celesta hit 0.1s
//    delay(2857/16); // 1/4 = 84bpm 2.857s/beat);
//    noTone(8);
//  }
}

uint32_t tt,t0,t;
uint16_t t1=120;
uint16_t t2=0;
int thisNote = 0;
void loop()
{
  t=micros();
  if (t-t0 >= t1) { digitalWrite(8,0); }
  if (t2!=0 && t-t0 >= t2) { digitalWrite(8,1); t0=t; t1--; }
  if (t-tt >= 178563) { tt = t; //NNA
    int freq = notes[pgm_read_byte_near(melody+thisNote)];
    if (thisNote < M_SZ-1)    thisNote++;
    if (freq == 0) t2=0;
    else t2 = (uint16_t)((long)1000000L/(long)freq);
    t0 = t; t1=120;
  }
}
