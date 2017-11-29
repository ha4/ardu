#include <SPI.h>
#include "Wire.h"
#include "owire.h"

#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,2);  // i2c display#2:A4-SDA->15pcf8574 #1:A5-SCL->14pcf8574 #3:+5v #4:GND
OneWire  ds(5); // on D0
const int chipSelectPinADC = 9; // 7clk<-sck=D13, 6Dout->MISO=D12, MOSI-nc=D11, 5CS<-D9

double tinp,uinp;

/* Thermometry data
/* http://www.mosaic-industries.com/embedded-systems/microcontroller-projects/temperature-measurement/thermocouple/calibration-table */

/* RTD Pt100 data. European curve. tollerance +/-0.015C. 100Ohm 273.15K
/*
float rtd2c = {
	18.52, 390.48, 0, -245.19, // -200,+850
	2.5293, -6.6046E-2, 4.0422E-3, -2.0697E-6,
	1,      -2.5422E-2, 1.6883E-3, -1.3601E-6,
};
*/


/* Type-J thermocouple conversion. Iron/Constantan. I/C. Iron/CopperNickel
   IEC extension cable: black(+)white(-)(black coat&connector)
   former extn.: UK:yellow+blue(black) USA:white+red(black) GER:red+blue(blue) */
float j2c[] = {
	-8.095, 0.000, -3.1169773E+00, -6.4936529E+01,
	2.2133797E+01, 2.0476437E+00, -4.6867532E-01, -3.6673992E-02,
	1,             1.1746348E-01, -2.0903413E-02, -2.1823704E-03,

	0.000, 21.840, 1.3592329E+01, 2.5066947E+02,
	1.8014787E+01, -6.5218881E-02, -1.2179108E-02, 2.0061707E-04,
	1,             -3.9494552E-03, -7.3728206E-04, 1.6679731E-05,

	21.840, 45.494, 3.6040848E+01, 6.4950262E+02,
	1.6593395E+01, 7.3009590E-01, 2.4157343E-02, 1.2787077E-03,
	1,             4.9172861E-02, 1.6813810E-03, 7.6067922E-05,

	45.494, 57.953, 5.3433832E+01, 9.2510550E+02,
	1.6243326E+01, 9.2793267E-01, 6.4644193E-03, 2.0464414E-03,
	1,             5.2541788E-02, 1.3682959E-04, 1.3454746E-04,

	57.953, 69.553, 6.0956091E+01, 1.0511294E+03,
	1.7156001E+01, -2.5931041E+00, -5.8339803E-02, 1.9954137E-02,
	1,             -1.5305581E-01, -2.9523967E-03, 1.1340164E-03,
};

float j2v[] = {
	-20, 70, 2.5000000E+01, 1.2773432E+00, // Xlow, Xhigh, X-offset[C], Y-offset[mV]
	5.1744084E-02, -5.4138663E-05, -2.2895769E-06, -7.7947143E-10,
	1,             -1.5173342E-03, -4.2314514E-05, 0
};

/* Type-K thermocouple conversion. Cromel/Alumel. C/A. NickelCromium/NickelAluminium. NC/NA.
   IEC extension cable: green(+)white(-)(green coat&connector)
   former extn.: UK:brown+blue(red) USA:yellow+red(yellow) GER:red+green(green)*/
float k2c[] = {
	-6.404, -3.554, -4.1790858E+00, -1.2147164E+02,
	3.6069513E+01, 3.0722076E+01, 7.7913860E+00, 5.2593991E-01,
	1,             9.3939547E-01, 2.7791285E-01, 2.5163349E-02,

	-3.554, 4.096, -3.4489914E-01, -8.7935962E+00,
	2.5678719E+01, -4.9887904E-01, -4.4705222E-01, -4.4869203E-02,
	1,              2.3893439E-04, -2.0397750E-02, -1.8424107E-03,

	4.096, 16.397, 1.2631386E+01, 3.1018976E+02,
	2.4061949E+01, 4.0158622E+00, 2.6853917E-01, -9.7188544E-03,
	1,             1.6995872E-01, 1.1413069E-02, -3.9275155E-04,

	16.397, 33.275, 2.5148718E+01, 6.0572562E+02,
	2.3539401E+01, 4.6547228E-02, 1.3444400E-02, 5.9236853E-04,
	1,             8.3445513E-04, 4.6121445E-04, 2.5488122E-05,

	33.275, 69.553, 4.1993851E+01, 1.0184705E+03,
	2.5783239E+01, -1.8363403E+00, 5.6176662E-02, 1.8532400E-04,
	1,             -7.4803355E-02, 2.3841860E-03, 0.0000000E+00,
};

float k2v[] = {
	-20, 70, 2.5000000E+01, 1.0003453E+00, // Xlow, Xhigh, X-offset[C], Y-offset[mV]
	4.0514854E-02, -3.8789638E-05, -2.8608478E-06, -9.5367041E-10,
	1,             -1.3948675E-03, -6.7976627E-05, 0
};

/* Type-T thermocouple conversion. Copper/Constantan. Cu/Con. Copper/CopperNickel
   IEC extension cable: brown(+)white(-)(brown coat&connector)
   former extn.: UK:white+blue(blue) USA:blue+red(blue) GER:red+brown(brown) */
float t2c[] = {
	-6.18, -4.648, -5.4798963E+00, -1.9243000E+02,
	5.9572141E+01, 1.9675733E+00, -7.8176011E+01, -1.0963280E+01,
	1,             2.7498092E-01, -1.3768944E+00, -4.5209805E-01,

	-4.648, 0.000, -2.1528350E+00, -6.0000000E+01,
	3.0449332E+01, -1.2946560E+00, -3.0500735E+00, -1.9226856E-01,
	1,              6.9877863E-03, -1.0596207E-01, -1.0774995E-02,

	0.000, 9.288, 5.9588600E+00, 1.3500000E+02,
	2.0325591E+01, 3.3013079E+00, 1.2638462E-01, -8.2883695E-04,
	1,             1.7595577E-01, 7.9740521E-03, 0.0000000E+00,

	9.288, 20.872, 1.4861780E+01, 3.0000000E+02,
	1.7214707E+01, -9.3862713E-01, -7.3509066E-02, 2.9576140E-04,
	1,             -4.8095795E-02, -4.7352054E-03, 0.0000000E+00

};

float t2v[] = {
	-20, 70, 2.5000000E+01, 9.9198279E-1,  // Xlow, Xhigh, X-offset[C], Y-offset[mV]
        4.0716564E-02, 7.1170297E-04, 6.8782631E-07, 4.3295061E-11, // P-numerator
        1.0,           1.6458102E-02, 0,             0              // Q-denominator
};


double poly4(float *a, double x) { return a[0] + x*(a[1] + x*(a[2] + x*a[3])); }
double ratpoly4(float *a, double x) { return x * poly4(a, x) / poly4(a+4, x); }
double pwrp4(float *t, int sz, double x) /* picewise ration polynomial 4-degree */
{
	int i=0;
	for(; i < sz; i++, t+=12) if (x<t[0] || (x>=t[0] && x<t[1])) break;
	if (i==sz) t-=12;
	if (x<t[0]) x=t[0]; else if (x>t[1]) x=t[1];
	return ratpoly4(t+4, x-t[2])+t[3];
}

/* input: C, output mV */
double convT2V(double T) { return ratpoly4(k2v+4, T-t2v[2])+t2v[3]; }

/* input: mV, output C */
double convV2T(double V) { return pwrp4(k2c, 5, V); }


uint16_t ref_t()
{
	byte data[12];
	uint16_t rc = 0xFFFF;

	if (ds.reset()) {
		ds.write(0xCC);     // skip rom
		ds.write(0xBE);         // Read Scratchpad
		ds.read(data, 9);
		if (ds.crc(data, 9)==0)
			rc = data[0] + 256*data[1];
		// restart conversion
		ds.reset();
		ds.write(0xCC);     // skip rom
		ds.write(0x44);     // start conversion
		ds.power(true);
	}

	return rc;
}

uint16_t read_mcp3201()
{
	uint16_t result;

	digitalWrite(chipSelectPinADC, LOW);
	result = SPI.transfer(0x00) << 8;
	result = result | SPI.transfer(0x00);
	digitalWrite(chipSelectPinADC, HIGH);
	result = result >> 1;
	result = result & 0xFFF;
  
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
	digitalWrite(chipSelectPinADC, HIGH);
}
 

#define ADC_OFFS   1.7    /*code*/
#define ADC_REF    4072.5 /*mV*/
#define AMP_SHIFT  516.0  /*521.2mV*/
#define AMP_FACTOR 1.830936e-2 /* A=54.61687 -1.67175% nominal=122.2/2.2=55.(54) */
#define BAT_REF    1.076  /* V, nominal 1.1V -2.18%*/
#define BAT_FACTOR 4.0121 /* K=4 +0.3025%. 300->1.266v 315->1.328 Ktot=4.21584e-3 */

char buf[32];

void loop()
{
	double mv,te;
	byte n;
	uint16_t result;

	analogRead(0);
	result=analogRead(0);
        mv=result*BAT_REF*BAT_FACTOR/1024;
	Serial.print(mv);
	Serial.print(' ');
	lcd.setCursor(0,1);
        lcd.print("Vb");
        lcd.print(mv,3);
  
	for(mv=0, n=8; n--;) {
		result=read_mcp3201();
		mv += result;
	}
	mv/=8;
        mv = mv * ADC_REF / 4096 + ADC_OFFS;

	Serial.print(mv);
	Serial.print(' ');
	mv = AMP_FACTOR * (mv-AMP_SHIFT);

	Serial.print(mv,3);
	Serial.print(' ');

	if((result=ref_t()) != 0xFFFF) {
		tinp=result/16.0;
		uinp = convT2V(tinp);
        	Serial.print(tinp);
        	lcd.setCursor(10,1);
                lcd.print("Tenv");
                lcd.print(tinp,3);
	} else
		Serial.print('-');

	Serial.print(' ');
	Serial.print(uinp,3);
	Serial.print(' ');

	te = convV2T(mv+uinp);
	Serial.println(te);
	lcd.setCursor(0,0); lcd.print('T'); 
        lcd.print(dtostrf(te,7,2,buf));
        lcd.print(" mV");
        lcd.print(dtostrf(mv,0,3,buf));
	delay(1000);
}
