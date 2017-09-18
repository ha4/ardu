//#include "SerialSend.h"
//#define Ser mySerial
//SerialSend mySerial(1);

#include <SoftwareSerial.h>
#define Ser mySerial
SoftwareSerial mySerial(2, 1); // RX, TX

enum {tcouple=A3, pot=A2, output=0, 
    out_inv=1, vref=1100, vref_corr=-88/*1.003*/, amp=55, amp_corr=-291/*39*/, pot_250=11625, pot_low=6202, pot_hi=22526, off_corr=-16 };


#include <EEPROM.h>
byte eerd(int a) { return EEPROM.read(a); }
void eewr(int a, byte d) { EEPROM.write(a,d); }

//#define Ser Serial
//enum {tcouple=A1, pot=A0, output=9,
//    out_inv=1, vref=1100, vref_corr=-17/*1.081*/, amp=55, amp_corr=30/*56.6*/, pot_250=11625, pot_low=6202, pot_hi=22526, off_corr=0 };

// working values
uint32_t sec;
uint8_t acc, pwr;
int16_t tcx,potx,tcu;
int16_t tset;
int16_t cv,prev_cv;
int32_t I;

#define CRC_START 0xFFFF
uint16_t crc;

// non-reserved param
uint8_t en_pid,en_pot,en_disp;

// reserved param
#define PAR_N 7
int16_t eparam[PAR_N];
#define DBVERSION 2
#define dbv eparam[0]
#define pb eparam[1]
#define si eparam[2]
#define sd eparam[3]
#define ampc eparam[4]
#define vrefc eparam[5]
#define offc eparam[6]


void pdefault()
{
  pb=841; si=20; sd=0; I=0;
  ampc=amp_corr;
  vrefc=vref_corr;
  offc=off_corr;
  dbv = DBVERSION;
}

void crcEEprom()
{
  crc = CRC_START;
  for(int i=2, j=0; j < PAR_N; j++) {
    calculateCRC(eerd(i++));
    calculateCRC(eerd(i++));
  }
}

void pwrite()
{
  int i;
  byte* p = (byte*)(void*)eparam;
  dbv = DBVERSION;
  for(int j=0, i=2; j < PAR_N; j++) {
      eewr(i++, *p++);
      eewr(i++, *p++);
  }

  crcEEprom();
  eewr(0, lowByte(crc));
  eewr(1, highByte(crc));
  Ser.println(crc,HEX);
}

int pload()
{
  int i;
  byte* p = (byte*)(void*)eparam;

  crcEEprom();
  Ser.println(crc,HEX);
  if (crc != word(eerd(1), eerd(0))) return 0;
  i=2;
  *p++ = eerd(i++);
  *p++ = eerd(i++);
  if (dbv != DBVERSION) return 0;

  for(int j=1; j < PAR_N; j++) {
          *p++ = eerd(i++);
          *p++ = eerd(i++);
  }
  return 1;
}

void setup()
{
  Ser.begin(9600);
  pinMode(output,OUTPUT);
  pwr=128;
  tset=pot_250;
  en_pid=1;
  en_pot=1;
  en_disp=1;
  pdefault();
  if(!pload()) Ser.println("Invalid EEPROM");
}

void disp() {
  Ser.print(cv);
  Ser.print(' ');
  Ser.print(tset);
  Ser.print(' ');
  Ser.println(pwr);
}

void pid()
{
  int32_t r,e,ee,imax;

  e = tset - cv;
  ee= cv-prev_cv;
  prev_cv=cv;
  imax = 256*si; // I-limit
  if (pb == 0) return;
  r = I + e*256L/((int32_t)pb); // integrate
  if (r < -imax) r=-imax; else if (r > imax) r=imax; // I-saturation
  I=r; if (si == 0) r=0; else r/=si; // I-part
  r += (e*256L - sd*ee*256L)/pb;     // P-part D-part
  if (r < 0) r = 0; else if (r > 255) r=255; // output saturation
  pwr=r;
}

void srecv()
{
  if (Ser.available())
    switch(Ser.read()) {
      case '?': Ser.print('e'); Ser.println(en_pid);
                Ser.print('r'); Ser.println(en_pot);
                Ser.print('d'); Ser.println(en_disp);
                Ser.print('a'); Ser.println(ampc);
                Ser.print('u'); Ser.println(vrefc);
                Ser.print('o'); Ser.println(offc);
                Ser.print("bp"); Ser.println(pb);
                Ser.print("bi"); Ser.println(si);
                Ser.print("bd"); Ser.println(sd);
                Ser.println("W[!Z]");
                Ser.print("I"); Ser.println(I);
                Ser.print("xT"); Ser.println(tcx);
                Ser.print("uT"); Ser.println(tcu);
                Ser.print("xP"); Ser.println(potx);
            break;
      default:
      err_default:
        Ser.println('?'); break;
      case 'p': pwr = Ser.parseInt(); break;
      case 't': tset = Ser.parseInt(); break;
      case 'e': en_pid = Ser.parseInt(); break;
      case 'r': en_pot = Ser.parseInt(); break;
      case 'd': en_disp = Ser.parseInt(); break;
      case 'a': ampc = Ser.parseInt(); break;
      case 'u': vrefc = Ser.parseInt(); break;
      case 'o': offc = Ser.parseInt(); break;
      case 'W': switch(Ser.read()) {
          case '!': pwrite(); break;
          case 'Z': pdefault(); break;
          default: goto err_default;
          }

      case 'b': switch(Ser.read()) {
          case 'p': pb= Ser.parseInt(); break;
          case 'i': si= Ser.parseInt(); break;
          case 'd': sd= Ser.parseInt(); break;
          default: goto err_default;
          }
    }
}

void tcread()
{
  int32_t v;
  analogReference(INTERNAL);
  tcx = analogRead(tcouple);
  delay(50);
  tcx = analogRead(tcouple);
  v = vref*(1000L + vrefc);
  v /= 100L;
  v *= tcx;
  v /= 10240;  // mV = x/1024 * Vref*(1+refc/1000)
  v += offc;
  tcu=v;
  v = v*100000L;
  v /= (amp*100+amp*ampc/10); // uV = mV * 1000 / [amp * (1+ampc/1000)]
  cv = v;
}

void potread()
{
  analogReference(DEFAULT);
  potx = analogRead(pot);
  delay(5);
  potx = analogRead(pot);
  tset = map(potx, 0,1024, pot_low,pot_hi);
}

uint8_t dtimer(int dt)
{
  uint32_t ms = millis();
  if (ms > sec+dt) {sec=ms; return 1;}
  return 0;
}

void pwrout()
{
  uint8_t t;
  t = acc+pwr;
  digitalWrite(output, (acc>t)?0:1);
  acc=t;
}

void loop()
{
  srecv();
  if (dtimer(250)) {
      tcread();
      if (en_pot) potread();
      if (en_pid) pid(); else I=0;
      pwrout();
  }
  if(en_disp) disp();      
  delay(100);
}

/* CRC16 Definitions */

static const uint16_t crc_table[16] = {
  0x0000, 0xcc01, 0xd801, 0x1400, 0xf001, 0x3c00, 0x2800, 0xe401,
  0xa001, 0x6c00, 0x7800, 0xb401, 0x5000, 0x9c01, 0x8801, 0x4400
};

uint16_t calculateCRC(byte x) 
{ 
  crc ^= x;
  crc = (crc >> 4) ^ crc_table[crc & 0x0f];
  crc = (crc >> 4) ^ crc_table[crc & 0x0f];
  return crc; 
}


