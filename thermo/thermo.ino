#include <AD770X.h>
#include <OneWire.h>

AD770X ad7705(1087.14);
OneWire  bus1(19);  // on pin PC5
enum { cnum = 10 }; // calibration counter

long ref_time;
float ref_temp, T;
float ref_mv;

double lpf;
double rlpf;

float lowpass(double *acc, float v)
{
    *acc = (*acc) * 0.98 + v*0.02;
    return *acc;
}

// t=-20C to +70C, u=-0.757mv to 2.909mv
float t1cp[]= { 4.0716564E-02, 7.1170297E-04, 6.8782631E-07, 4.3295061e-11 };
float t1cq[]= {             1, 1.6458102E-02,             0,             0 };

float t1r[] = {-5.4798963, -192.43, -4.648 }; //-6.18;-4.648mV -250;-150°C 	
float t1p[] = { 5.9572141E+01, 1.9675733E+00,-7.8176011E+01,-1.0963280E+01 };
float t1q[] = {             1, 2.7498092E-01,-1.3768944E+00,-4.5209805E-01 };
float t2r[] = {-2.152835,   -60.0,   0.0   }; //-4.648;0mV  -150;0C
float t2p[] = { 3.0449332E+01,-1.2946560E+00,-3.0500735E+00,-1.9226856E-01 };
float t2q[] = {             1, 6.9877863E-03,-1.0596207E-01,-1.0774995E-02 };
float t3r[] = { 5.95886,    135.0,   9.288 }; //0;9.288mV 0;200°C
float t3p[] = { 2.0325591E+01, 3.3013079E+00, 1.2638462E-01,-8.2883695E-04 };
float t3q[] = {             1, 1.7595577E-01, 7.9740521E-03,           0.0 };
float t4r[] = {14.86178,    300.0,  20.872 }; //9.288;20.872mV 200;400°C
float t4p[] = { 1.7214707E+01,-9.3862713E-01,-7.3509066E-02, 2.9576140E-04 };
float t4q[] = {             1,-4.8095795E-02,-4.7352054E-03,           0.0 };

float poly3(float *a, float x)
  { return a[0]+x*(a[1]+x*(a[2]+x*a[3])); }

float typeTp(float *p, float *q, float vr)
{    float pt,qt;
    pt = vr*poly3(p,vr);
    qt =    poly3(q,vr);
    return pt/qt;
}

float typeT(float u)
{
  if (u < t1r[2]) return t1r[1]+typeTp(t1p, t1q, u-t1r[0]);
  if (u < t2r[2]) return t2r[1]+typeTp(t2p, t2q, u-t2r[0]);
  if (u < t3r[2]) return t3r[1]+typeTp(t3p, t3q, u-t3r[0]);
                  return t4r[1]+typeTp(t4p, t4q, u-t4r[0]);;
}

float typeT_Vcold(float t)
{
  return 0.9919828+typeTp(t1cp,t1cq,t - 25.0); 
}


void reftemp()
{
  byte data[12];
  byte i;
  float t;

  if (millis() <= ref_time) return;

  if (!bus1.reset()) 
     { goto no_exist; }

  bus1.write(0xCC);  // skip ROM
  bus1.write(0xBE);  // Read Scratchpad
  bus1.vread(data, 9);
  i = OneWire::crc8(data, 8);
 
  if (i == data[8]) {
      t = data[0];
      t += 256.0*data[1];
      t /= 16.0;
      if (t < 84.0) {
        if (rlpf <= -300) rlpf = t;
            else lowpass(&rlpf, t);
        ref_mv = typeT_Vcold(rlpf);
      }
  }

  bus1.reset();
  bus1.write(0xCC);  // skip ROM
  bus1.write(0x44); // start conversion, with parasite power on at the end
  bus1.power(true);
no_exist:
  ref_time = millis()+800;
}

long send24(uint8_t reg, long s)
{
    long r;
    ad7705.cs();
    ad7705.spi_send(reg);
    r = ad7705.spi_send((s>>16)&0xFF);
    r = (r << 8) | ad7705.spi_send((s>>8)&0xFF);
    r = (r << 8) | ad7705.spi_send(s&0xFF);
    ad7705.nocs();
    return r;
}

void setup()
{
    long z1, g1;
    int i;

    Serial.begin(9600);

    // PB1, OC1A, D9 pin 2MHz clock
    //       mode 0100 (CTC),top=OCR1A, COM1A mode=1 (toggle), src=001
    TCCR1A = (0b01 << COM1A0)| (0b00 << WGM10); // div 2
    TCCR1B = (0b01 << WGM12) | (0b001 << CS10); // clk/1 (16MHz)
    OCR1A = 4 - 1; // div 4
    pinMode(9, OUTPUT);

    analogReference(INTERNAL);
    analogRead(A7);

    ad7705.reset();

    ad7705.init(AD770X::CHN_AIN1, AD770X::BIPOLAR, AD770X::GAIN_1);
    delay(250);

    z1 = 0;
    g1 = 0;
    for (i=cnum; i--; ) {
      ad7705.command(AD770X::REG_SETUP|AD770X::CHN_AIN1|AD770X::REG_WR, 
        AD770X::MODE_SELF_CAL|AD770X::BIPOLAR|AD770X::GAIN_1);
      delay(10+6*40);
      z1 += send24(AD770X::REG_OFFSET|AD770X::CHN_AIN1|AD770X::REG_RD,0xFFFFFF);
      g1 += send24(AD770X::REG_GAIN | AD770X::CHN_AIN1|AD770X::REG_RD, 0xFFFFFF);
    }
    z1 /= cnum;
    g1 /= cnum;

    // calibration correction
    z1 += 3166;
    g1 += g1/1251;

    send24(AD770X::REG_OFFSET|AD770X::CHN_AIN1|AD770X::REG_WR, z1);
    send24(AD770X::REG_GAIN | AD770X::CHN_AIN1|AD770X::REG_WR, g1);
}  

void loop()
{
  float v, T;
  uint16_t h;

  delay(40);
  ad7705.wait(AD770X::CHN_AIN1);
  h = ad7705.read16(AD770X::REG_DATA|AD770X::REG_RD|AD770X::CHN_AIN1);
  v = ad7705.conv(h) / 100.0; // amplifier coefficient
  v = lowpass(&lpf,v);

  T = typeT(v+ref_mv);

  Serial.print(h);  Serial.print('\t');  Serial.print(v, 4);  Serial.print('\t');
  Serial.print(ref_mv, 4);  Serial.print('\t');  Serial.println(v+ref_mv, 2);

  reftemp();
}
