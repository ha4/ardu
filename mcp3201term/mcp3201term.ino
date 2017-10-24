#include <SPI.h>
#include "Wire.h"
#include "owire.h"

#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,2);  // i2c display#2:A4-SDA->15pcf8574 #1:A5-SCL->14pcf8574 #3:+5v #4:GND

OneWire  ds(5); // on D0

const int chipSelectPinADC = 9; // 7clk<-sck=D13, 6Dout->MISO=D12, MOSI-nc=D11, 5CS<-D9

double tinp,uinp;

/* Type-T thermocouple conversion */
/* http://www.mosaic-industries.com/embedded-systems/microcontroller-projects/temperature-measurement/thermocouple/calibration-table */

double convV2T1(double Vr) {
   double P = Vr*(59.572141+Vr*(1.9675733 +Vr*(-78.176011-Vr*10.963280)));
   double Q =     1.0 +     Vr*(0.27498092+Vr*(-1.3768944-Vr*0.45209805));
   return P/Q;
}

double convV2T2(double Vr) {
   double P = Vr*(30.449332 + Vr*(-1.2946560   +Vr*(-3.0500735-Vr*0.19226856)));
   double Q =     1.0       + Vr*(6.9877863E-03+Vr*(-0.10596207-Vr*1.0774995E-02));
   return P/Q;
}

double convV2T3(double Vr) {
   double P = Vr*(20.325591+Vr*(3.3013079  + Vr* (0.12638462 - Vr*8.2883695E-04)));
   double Q =     1.0      +Vr*(0.17595577 + Vr*7.9740521E-03);
   return P/Q;
}

double convV2T4(double Vr) {
   double P = Vr*(17.214707 + Vr*(-0.93862713    + Vr*(-0.073509066+Vr*2.9576140E-04)));
   double Q =     1.0       + Vr*(-4.8095795E-02 - Vr*4.7352054E-03);
   return P/Q;
}

double convV2T(double V) { /* input: mV, output C */
	if(V <= -4.648) return convV2T1(V+5.4798963)-192.43;
	if(V <= 0)  return convV2T2(V+2.1528350)-60.0;
	if(V <= 9.288) return convV2T3(V-5.9588600)+135.0;
	return convV2T4(V-14.861780)+300.0;
}

double poly(float *a, double x)
{
  return a[0] + x*(a[1] + x*(a[2] + x*a[3]));
}

double ratpoly4(float *a, double x)
{
  double xx = x-a[0];
  return xx * poly(a + 2, xx) / poly(a+6, xx) + a[1];
}

float t2v[] = {2.5000000E+01, 9.9198279E-1, // X-offset, Y-offset
               4.0716564E-02, 7.1170297E-04, 6.8782631E-07, 4.3295061E-11, // P-numerator
               1.0,           1.6458102E-02, 0,             0};            // Q-denomterator

double convT2V(double T) { /* input: C, output mV */
  return ratpoly4(t2v, T);
}

byte ref_t()
{
  byte data[12];
  int  tu;
  byte rc = 0;

  if (ds.reset()) {
      ds.power(false);
      ds.write(0xCC);     // skip rom
      ds.write(0xBE);         // Read Scratchpad
      ds.read(data, 9);
      if (ds.crc(data, 9)==0) {
        tu = data[0] + 256*data[1];
        tinp = (double)tu/16;
        uinp = convT2V(tinp);
        rc=1;
      }
  }
 ds.reset();
 ds.write(0xCC);     // skip rom
 ds.write(0x44);     // start conversion
 ds.power(true);
 return rc;
}

unsigned int read_mcp3201()
{
  unsigned int result;
  byte inByte;

  digitalWrite(chipSelectPinADC, LOW);
  result = SPI.transfer(0x00);
  result = result << 8;
  inByte = SPI.transfer(0x00);
  result = result | inByte;
  digitalWrite(chipSelectPinADC, HIGH);
  result = result >> 1;
  result = result & 0b0000111111111111;
  
  return result;
}

void setup()
{
Serial.begin(9600);
lcd.init();
lcd.print("loading..");
tinp=0;
uinp=0;
analogReference(INTERNAL);
analogRead(0);
analogRead(0);
SPI.begin();
SPI.setBitOrder(MSBFIRST);
SPI.setClockDivider(SPI_CLOCK_DIV16);
pinMode(chipSelectPinADC, OUTPUT);
pinMode(6, OUTPUT);
pinMode(8, OUTPUT);
digitalWrite(6, LOW); /// ground
digitalWrite(8, HIGH); /// ground
digitalWrite(chipSelectPinADC, HIGH);
}
 

void loop()
{
  double mv,te;
  byte n;
  unsigned int result;

  
  for(mv=0, n=8; n--;) {
    result=read_mcp3201();
    mv += result;
//  Serial.print(result); // mv
//  Serial.print(' ');
  }
  mv/=8;
  mv=map(mv*10, 0, 40960, 17, 17+40725)/10;
  Serial.print(mv);
  Serial.print(' ');
  mv = 0.01830936 * (mv-516.0);

  Serial.print(mv,3);
  Serial.print(' ');

  if(ref_t()) {
        Serial.print(tinp);
 } else
       Serial.print('-');
 Serial.print(' ');
 Serial.print(uinp,3);
 Serial.print(' ');

 te = convV2T(mv+uinp);
 Serial.println(te);
 lcd.setCursor(0,0);
 lcd.print(te); lcd.print(' ');
delay(1000);
}

void loop0()
{
  unsigned int result;
  double mv;

  result=read_mcp3201();
  result=map(result*10, 0, 40960, 17, 40761)/10;
  mv = -9.288 + 0.01830935 * result;
  Serial.print(result);
  Serial.print(' ');
  Serial.println(mv,3);
  
  Serial.flush();
}
