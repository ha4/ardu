#include <util/atomic.h>
#include "bayang_rx.h"

//#define DEBUG
//#define USE_PPM
#define USE_SBUS
//#define USE_FAILSAFE

#define IO_pin    2

#define IO_off   pinMode(IO_pin, INPUT_PULLUP)
#define IO_on   {pinMode(IO_pin, OUTPUT); digitalWrite(IO_pin, 0);}
#define IO_read  digitalRead(IO_pin)

struct BayangData data;

#ifdef USE_SBUS
#include "sbus.h"
extern sbusChannels_t sbus;
#endif

#ifdef USE_PPM
#include "ppm.h"
extern int ppm[];
#endif

bool no_signal;
uint32_t ref_t;
uint32_t timout;
uint32_t tsbus;
uint32_t tscan;

void setup()
{
    pinMode(IO_pin, INPUT_PULLUP);

#ifdef DEBUG
    Serial.begin(250000);
    Serial.println("starting radio");
#endif

#ifdef USE_PPM
    setupPPM();
#endif

#ifdef USE_SBUS
    sbus_start();
#endif

    BAYANG_RX_data(&data);
    data.option=0; /// BAYANG_OPTION_BIND;
    BAYANG_RX_init();
    ref_t=micros();
    tsbus=ref_t;
    tscan=ref_t;
    timout=0;
    no_signal=1;
}

void loop()
{
    uint32_t t=micros();

    if(t-ref_t >= timout) {
      ref_t = t;
      timout=BAYANG_RX_callback();
    }

    if(BAYANG_RX_available()) {
#ifdef DEBUG
      Serial.print(data.lqi);Serial.print(' ');
      Serial.print(data.roll);Serial.print(' ');
      Serial.print(data.pitch);Serial.print(' ');
      Serial.print(data.throttle);Serial.print(' ');
      Serial.print(data.yaw);Serial.println();
#endif
#ifdef USE_PPM
      setPPMValuesFromData();
#endif
    }

#ifdef USE_SBUS
    if(t-tsbus >= SBUS_HIGHSPEED_PERIOD) { // decimate
      tsbus=t;
      setSBUSValuesFromData();
      sbus_send();
    }
#endif

    // button check & state 5ms scan time
    if(t-tscan >= 5000) {
      tscan=t;
      IO_off;
      scan_button();
      switch(BAYANG_RX_state()) {
        case 1: IO_on; no_signal=0; break; // in sync
        case 0: led_flash(0xAA); no_signal=1; break; // binding
        default: led_flash(0x01); no_signal=1; break; // data loss
      }
    }
}

uint8_t btncnt,btnstat;
int8_t btnflt;
void scan_button()
{
  // debounce
  int8_t j=!IO_read;
  btnflt += j+j - ((btnflt)>>2); // alphafilter y=0.75y+0.25x: y=y-0.25y+0.25x: y+=0.25x-0.25y: 8y=2x-2y
  if (btnflt > 4) j=0; else j=1;
  // changestate
  if (btnstat == j) {
    if(btncnt != 255) btncnt=255; // 5ms*255=1.275s 
    return;
  }
  btnstat = j;

  if (j) { // release
    if (btncnt >= 200) BAYANG_RX_bind(); // 1s press
  }
  btncnt=0;
}

uint8_t flashcnt;
uint8_t flashmsk;
void led_flash(uint8_t pattern)
{
  if ((flashmsk & pattern)!=0) IO_on;
  if (++flashcnt < 10) return; // 10*5 = 50ms bit rate, 400ms period, 2.5Hz
  flashcnt = 0;
  flashmsk <<=1;
  if (flashmsk == 0) flashmsk=1;
}

/*  channels from Multiprotocol-TX
 *  ch1 ch2 ch3 ch4 ch5  ch6 ch7 ch8 ch9 ch10 ch11   ch12   ch13    ch14 ch15
 *   A   E   T   R  flip rth pic vid head inv dyntrm takoff emgstop aux1 aux2
 */

#ifdef USE_PPM
void set_ppm(chan, val)
{
  ATOMIC_BLOCK(ATOMIC_FORCEON) { // faster
    ppm[chan]=val;  
  }
}

void setPPMValuesFromData()
{
  set_ppm(0, map(data.roll,     0, 1023, 1000, 2000));  // aileron
  set_ppm(1, map(data.pitch,    0, 1023, 1000, 2000));  // elevator
  set_ppm(2, map(data.throttle, 0, 1023, 1000, 2000));  // throttle
  set_ppm(3, map(data.yaw,      0, 1023, 1000, 2000));  // rudder
  set_ppm(4, (data.flags2&BAYANG_FLAG_FLIP)?2000:1000));
  set_ppm(5, (data.flags2&BAYANG_FLAG_VIDEO)?2000:1000));
  set_ppm(6, map(data.aux1,     0,  255, 1000, 2000));
  set_ppm(7, map(data.aux2,     0,  255, 1000, 2000));
}
#endif

#ifdef USE_SBUS
void setSBUSValuesFromData()
{
  sbus.chan0 = map(data.roll,     0, 1023, 192, 1971); // aileron
  sbus.chan1 = map(data.pitch,    0, 1023, 192, 1971); // elevator
  sbus.chan2 = map(data.throttle, 0, 1023, 192, 1971); // throttle
  sbus.chan3 = map(data.yaw,      0, 1023, 192, 1971); // rudder
  sbus.chan4 = (data.flags2&BAYANG_FLAG_FLIP)?1971:192;
  sbus.chan5 = (data.flags2&BAYANG_FLAG_RTH)?1971:192;
  sbus.chan6 = (data.flags2&BAYANG_FLAG_PICTURE)?1971:192;
  sbus.chan7 = (data.flags2&BAYANG_FLAG_VIDEO)?1971:192;
  sbus.chan8 = (data.flags2&BAYANG_FLAG_HEADLESS)?1971:192;
  sbus.chan9 = (data.flags3&BAYANG_FLAG_INVERTED)?1971:192;
  sbus.chan10 = 192;
  sbus.chan11 = (data.flags3&BAYANG_FLAG_TAKE_OFF)?1971:192;
  sbus.chan12 = (data.flags3&BAYANG_FLAG_EMG_STOP)?1971:192;
  sbus.chan13 = map(data.aux1,     0,  255, 192, 1971);
  sbus.chan14 = map(data.aux2,     0,  255, 192, 1971);
#ifdef USE_FAILSAFE
  sbus.flags = no_signal?SBUS_FLAG_SIGNAL_LOSS:0;
#else
  sbus.flags = 0;
#endif
}
#endif
