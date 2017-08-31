/*
 * PEM (Photo Electro Multiplier)
 * data acquestion software
 * external transistor correction:   T*.9612-15.29;
 * amp1 correction: 347.1305541*(u-(-1.251621925));
 * TlnT:  3345.8231*(1.10036274781-u);

 */

#include <AD7714.h>
#include <EEPROM.h>

enum {pwm_pin = 3, adc_cs=10 };

AD7714 adc(2.5,adc_cs);

int reg;     // temperature regulation
int ancal;   // analog calibration  ad7714 MD2,1,0
float gett;
float ipart;
float tprev;
float tt;

enum { pRegt=0, pKp, pKi, pTcorr_add, pTcorr_mul, pTconv_a, pTconv_b,
    pAmp_offs, pAmp_trans, PARMSF, pKzs=0, pKfs, PARMSL};

#define PARAMSZE (sizeof(struct paramstore)) 
struct paramstore  {
    uint16_t crc;
    float f[PARMSF];
    uint32_t l[PARMSL];
} ee;

// stored parameters thermo
// FLOAT
//0:regt Terr = regt - T
//1:kp;     // P = kp*Terr
//2:ki;     // I = ki*DT*accum(Terr) = accum(Terr*ki*DT)
//3,4:tcorr_add, tcorr_mul; // T = Tconv*tcorr_mul + tcorr_add;
//5,6:tconv_a, tconv_b;   // TlnT = tconva(tconv_b-adct);
//7:amp_offs; // u = adci-offs
//8:amp_trans; // i_pem = trans*u
// stored parameters 
// LONG
//0:kZS;  // ZS = ZSself + kZS
//1:kFS;  // FS = FSself + FSself*kFS/1000


void default_param()
{
  ee.f[pRegt] = -65;
  ee.f[pKp] = 10;
  ee.f[pKi] = 1.7;

  ee.f[pTcorr_mul] = 0.9612;
  ee.f[pTcorr_add] = -15.29;

  ee.f[pTconv_a] = 3345.8231;
  ee.f[pTconv_b] = 1.10036274781;
  
  ee.f[pAmp_offs] = -1.251621925;
  ee.f[pAmp_trans] = 347.1305541;

  ee.l[pKzs] = 0;
  ee.l[pKfs] = 0;
}

void store_param()
{
    int i;

    ee.crc = crc16(2+(uint8_t *)&ee, PARAMSZE-2);
    for(i=0; i < PARAMSZE; i++)
      EEPROM.write(i, ((uint8_t *)&ee)[i]);
}

uint8_t load_param()
{
    int i;
    uint16_t c1;
    struct paramstore tee;

    for(i=0; i < PARAMSZE; i++)
      ((uint8_t *)&tee)[i] = EEPROM.read(i);

    c1= crc16(2+(uint8_t *)&tee, PARAMSZE-2);
    if (tee.crc != c1)
        return 0;
    memmove(&ee,&tee,PARAMSZE);
    return 1;
}

void show_param()
{
  int i;
  Serial.println("param-start");
  for(i=0;i<PARMSF;i++) {
    if(i!=0) Serial.print(";");
    Serial.print("f");Serial.print(i);
    Serial.print(" ");Serial.print(ee.f[i],7);
  }
  for(i=0;i<PARMSL;i++) {
    if(i!=0) Serial.print(";");
    Serial.print("l");Serial.print(i);
    Serial.print(" ");Serial.print(ee.l[i]); 
  }
  Serial.print("crc ");Serial.println(ee.crc,HEX);
  Serial.print("param-end");
}


void setup()
{
  pinMode(pwm_pin, OUTPUT); // pin
  analogWrite(pwm_pin, 0);

  default_param();
  reg=load_param();

  Serial.begin(57600);

  ipart = 0;
  gett = 25;

  Serial.print(":start");
  if (reg) Serial.println("OK");
   else Serial.println("DEFAULT");

  reg = 1;

  adc.reset();
  adc.init(AD7714::CHN_16,AD7714::BIPOLAR,AD7714::GAIN_1, 1920);
  adc.self_calibrate(AD7714::CHN_56);
  Serial.println(":calibrated");
  tt = tprev = millis();
}

static const uint8_t regcal[8] = {
  AD7714::REG_DATA,   // mode 0 1t
  AD7714::REG_OFFSET, // mode 1 - selfcal, 6t
  AD7714::REG_OFFSET, // mode 2 - sys zero 3t
  AD7714::REG_GAIN,   // mode 3 - sys gain 3t
  AD7714::REG_OFFSET, // mode 4 - syszero/selfgain 6t
  AD7714::REG_DATA,   // mode 5 - background cal, 6t
  AD7714::REG_OFFSET, // mode 6 - zero self
  AD7714::REG_GAIN    // mode 7 - scale self
};

uint32_t a_calibrate(uint8_t chan)
{
  uint32_t cc;
  adc.command(AD7714::REG_MODE|chan|AD7714::REG_WR, (ancal << 5)|AD7714::GAIN_1);
  adc.wait(chan);
  cc = adc.read24(regcal[ancal]|chan|AD7714::REG_RD);
  return cc;
}

double TlnT(double T) { return (T <= 0) ? 0 : T*log(T); }
double stepTlnT(double T, double TlnT) { return -(T*log(T)-TlnT)/(1+log(T)); }
double invTlnT(double TlnT) {
double  T = 200; T += stepTlnT(T, TlnT); 
T += stepTlnT(T, TlnT); T += stepTlnT(T, TlnT); return T; }

void taloop(void)
{
  double v;

  tprev = tt;
  tt = millis();
  if (ancal) {
    Serial.print(a_calibrate(AD7714::CHN_12));
    return;
  }
  adc.fsync(AD7714::CHN_12);
  v = adc.read(AD7714::CHN_12);


  v = ee.f[pTconv_a]*(ee.f[pTconv_b]-v); //TlnT
  v = invTlnT(v) - 273.15;

  v = ee.f[pTcorr_mul]*v + ee.f[pTcorr_add]; //correction

  Serial.print(v,1); // single decimal digit
  Serial.print("C");

  gett = v;
}

void ialoop(void)
{
  double v;

  if (ancal) {
    Serial.print(a_calibrate(AD7714::CHN_56));
    return;
  }

  adc.fsync(AD7714::CHN_56);
  v = adc.read(AD7714::CHN_56);
  v = ee.f[pAmp_trans]*(v - ee.f[pAmp_offs]); // PEM current
  Serial.print(v,6);
}

void iloop(void)
{
  int x;
  float xx; uint32_t xxx;
 
  if (!Serial.available()) return;
  switch(Serial.read()){
    case 'p':    x = Serial.parseInt();    Serial.print("pwm");    Serial.print(x);    analogWrite(pwm_pin, x);    break;
    case 'r':    reg=Serial.parseInt();    Serial.print("reg");    Serial.print(reg);    break;
    case 'c':    ancal=Serial.parseInt();    Serial.print("analog-calibration");    Serial.print(ancal);    break;
    case 'l':    x=Serial.parseInt(); ee.l[x]=Serial.parseInt(); Serial.print("long"); Serial.print(x); Serial.print("="); Serial.print(ee.l[x]);    break;
    case 'f':    x=Serial.parseInt(); ee.f[x]=Serial.parseFloat();Serial.print("float"); Serial.print(x); Serial.print("="); Serial.print(ee.f[x]);    break;
    case '?':  show_param();     break;
    case 'Z':  default_param();    Serial.print("defaults-param");    break;
    case 'W':  x=Serial.parseInt();    if (x==1) { Serial.print("write-param"); store_param(); }    break;
  }
}

void trloop()
{
    float er, sum;
    uint8_t p1;

    er = (ee.f[pRegt]- gett);
    ipart += ee.f[pKi]*(tt-tprev)*er/1000.0; // time in ms

    if (ipart < -255) ipart = -255;
    if (ipart > 255) ipart = 255;

    sum = ee.f[pKp]*er + ipart;

    if (sum <0) sum = 0;
    if (sum > 253) sum=253;

    p1 = sum;

    analogWrite(pwm_pin, p1);
    Serial.print(p1);
}

void loop()
{
  adc.reset();  // spi interface reset, send 0xFFFFFFFF
  Serial.print(":");
  taloop();
  Serial.print(":");
  ialoop();
  Serial.print(":");
  iloop();
  Serial.print(":");
  if (reg) trloop();
  Serial.println(":>");
}


/* CRC16 Definitions */
static const uint16_t crc_table[16] = {
  0x0000, 0xcc01, 0xd801, 0x1400, 0xf001, 0x3c00, 0x2800, 0xe401,
  0xa001, 0x6c00, 0x7800, 0xb401, 0x5000, 0x9c01, 0x8801, 0x4400
};

uint16_t crc16(uint8_t *message, uint8_t length) 
{ 
  uint16_t crc; 
  uint8_t i; 

  crc=0xFFFF;
  for(i = 0; i != length; i++) 
    { 
	  crc ^= message[i];
	  crc = (crc >> 4) ^ crc_table[crc & 0xf];
	  crc = (crc >> 4) ^ crc_table[crc & 0xf];
    } 
  return crc; 
}
