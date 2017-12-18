#include <Arduino.h>
#include "thermo.h"

/* Thermometry data
/* http://www.mosaic-industries.com/embedded-systems/microcontroller-projects/temperature-measurement/thermocouple/calibration-table */

/* RTD Pt100 data. European curve. tollerance +/-0.015C. 100Ohm 273.15K R100C=138.51 */
float rtd2c[] = {
	18.52, 390.48, 0, -245.19, // -200,+850
	2.5293, -6.6046E-2, 4.0422E-3, -2.0697E-6,
	1,      -2.5422E-2, 1.6883E-3, -1.3601E-6,
};

float rtd2r[] = {
	-200, 850, 0, 100,
	0.3851, 0, 0, 0,
	1,      0, 0, 0,
};


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

static float *kc=k2c, *kv=k2v;
static int ktyp=TYP_K, ksz=5;

double poly3(float *a, double x) { return a[0] + x*(a[1] + x*(a[2] + x*a[3])); }
double ratpoly4(float *a, double x) { return x * poly3(a, x) / poly3(a+4, x); }
double pwrp4(float *t, int sz, double x) /* picewise ration polynomial 4-degree */
{
	int i=0;
	for(; i < sz; i++, t+=12) if (x<t[0] || (x>=t[0] && x<t[1])) break;
	if (i==sz) t-=12;
	if (x<t[0]) x=t[0]; else if (x>t[1]) x=t[1];
	return ratpoly4(t+4, x-t[2])+t[3];
}

/* input: C, output mV */
double convT2V(double T) { return ratpoly4(kv+4, T-kv[2])+kv[3]; }

/* input: mV, output C */
double convV2T(double V) { return pwrp4(kc, ksz, V); }

int conv_type(int typ)
{
	switch(typ) {
	case TYP_PT100:ktyp=TYP_PT100; kc=j2c; kv=j2v; ksz=1; break;
	case TYP_T: ktyp=TYP_T; kc=t2c; kv=t2v; ksz=4; break;
	case TYP_K: ktyp=TYP_K; kc=k2c; kv=k2v; ksz=5; break;
	case TYP_J: ktyp=TYP_J; kc=j2c; kv=j2v; ksz=5; break;
	}
	return ktyp;
}

