//
// uses an nRF24L01p connected to an Arduino
// 
// Cables are:
//     SS       -> 10
//     MOSI     -> 11
//     MISO     -> 12
//     SCK      -> 13
// 
// and CE       ->  9
//
//  2  4  6  8  front view
//  1  3  5  7
//
// GND CE CK MISO
//  1  3  5  7  edge view
//  ----------
//  2  4  6  8 INT 
// VCC CS MOSI 

#include <SPI.h>
#include <util/atomic.h>
#include "symax_nrf24l01_rx.h"
#include "sbus.h"
#include "ppm.h"
#include "xtimer.h"

#define USE_SERIALSTRING
#define USE_TIMING
//#define USE_PPM
//#define USE_SBUS


#ifdef USE_TIMING
#define _DEF_TIMING(x) x
#else
#define _DEF_TIMING(x)
#endif

enum { pinLED = 2 };

xtimer timer;
xtimer flsh;
uint8_t ppm_state;
uint16_t ppmvalues[8];
uint32_t yy;
struct SymaXData rxdata;

void setup()
{
#ifdef USE_SERIALSTRING
  Serial.begin(115200);
  while(!Serial);
  Serial.println(_DEF_TIMING("timing ")"Throttle Elevation Rudder Aileron");
#endif

#ifdef USE_SBUS
  sbus_start();
#endif
  pinMode(pinLED,OUTPUT);

  flsh.set(500);
  flsh.start(millis());
  timer.set(0);
  timer.start(micros());
#ifdef USE_PPM
  ppm_stop();
  ppm_state=0;
#endif
  symax_rx_data(&rxdata);
}

uint16_t byte2rc(int16_t b)
{ /* 0..255 -> 1000..2000 scale=125/32 */
  return 1000+((125*b)>>5); 
}

uint16_t byte2sbus(int16_t b)
{ /* 0..255 -> 192..1792 scale=25/4 */
  return 192+((25*b)>>2); 
}

void loop() 
{ 
  int rc;
  uint32_t dy,t;

  t=micros();
  if (timer.check(t))  timer.set(symax_rx_run());

  rc=symax_rx_binding();
  if (rc) {
    if (rc > 0) {
      if (flsh.check(millis())) digitalWrite(pinLED, 1-digitalRead(pinLED));
#ifdef USE_PPM
      if (ppm_state) { ppm_stop(); ppm_state=0; }
#endif
    }
    return;
  }
    digitalWrite(pinLED,0);

#ifdef USE_SBUS
  /* convert TREAF -> AETR1234 */
  sbus.chan0=byte2sbus(128-rxdata.aileron);
  sbus.chan1=byte2sbus(128+rxdata.elevator);
  sbus.chan2=byte2sbus(rxdata.throttle);
  sbus.chan3=byte2sbus(128-rxdata.rudder);
  sbus.chan4=(rxdata.flags5&FLAG5_HIRATE)?1792:192;
  sbus.chan5=(rxdata.flags6&FLAG6_AUTOFLIP)?1792:192;
  sbus.chan6=(rxdata.flags4&FLAG4_VIDEO)?1792:192;
  sbus.chan7=(rxdata.flags4&FLAG4_PICTURE)?1792:192;
  sbus_send();
#endif

#ifdef USE_PPM
  /* convert TREAF -> AETR1234 */
  ppmvalues[0]=byte2rc(128-rxdata.aileron);
  ppmvalues[1]=byte2rc(128+rxdata.elevator);
  ppmvalues[2]=byte2rc(rxdata.throttle);
  ppmvalues[3]=byte2rc(128-rxdata.rudder);
  ppmvalues[4]=(rxdata.flags5&FLAG5_HIRATE)?2000:1000;
  ppmvalues[5]=(rxdata.flags6&FLAG6_AUTOFLIP)?2000:1000;
  ppmvalues[6]=(rxdata.flags4&FLAG4_VIDEO)?2000:1000;
  ppmvalues[7]=(rxdata.flags4&FLAG4_PICTURE)?2000:1000;
  
  ATOMIC_BLOCK(ATOMIC_FORCEON)
    { for(int i=0;i<8;i++) ppm[i]=ppmvalues[i]; }

  if (!ppm_state) { ppm_start(); ppm_state=1; }
  //for(int i=0;i<8;i++) {Serial.print(ppmvalues[i]); Serial.print(' ');}
#endif

#ifdef USE_SERIALSTRING
  dy=t-yy;
  yy=t;
  _DEF_TIMING({Serial.print(dy);  Serial.print(' '); });
  
  Serial.print(rxdata.throttle);   Serial.print(' ');
  Serial.print(rxdata.elevator);   Serial.print(' ');
  Serial.print(rxdata.rudder);   Serial.print(' ');
  Serial.print(rxdata.aileron);
  Serial.println();
#endif
}
