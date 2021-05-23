#include "bayang_rx.h"

#define DEBUG
//#define USE_PPM
//#define USE_SBUS

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


uint32_t ref_t;
uint32_t timout;
uint32_t t1s;
uint32_t tsbus;
uint32_t tscan;
uint32_t pps,ppscnt;

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
    timout=BAYANG_RX_init();
    ref_t=micros();
    tsbus=ref_t;
    tscan=ref_t;
    t1s=ref_t;
}

void bind(uint32_t t)
{
  ref_t = t;
  timout = BAYANG_RX_bind();
}

void loop()
{
    uint32_t t=micros();

    if(t-ref_t >= timout) {
      ref_t = t;
      timout=BAYANG_RX_callback();
    }
    if(t-t1s >= 1000000L) {
      t1s=t;
      pps=ppscnt;
      ppscnt=0;
    }
    if(BAYANG_RX_available()) {
      ppscnt++;
#ifdef DEBUG
      Serial.print(pps);Serial.print(' ');
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
        case 1: IO_on; break; // in sync
        case 0: led_flash(0xAA); break; // binding
        default: led_flash(0x01); break; // data loss
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
#ifdef DEBUG
  Serial.print("button changed:");
  Serial.println(j);
#endif
  if (j) { // release
    if (btncnt >= 200) bind(micros()); // 1s press
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

#ifdef USE_PPM
void setPPMValuesFromData()
{
  ppm[0] = map(data.roll,     0, 1023, 1000, 2000);  
  ppm[1] = map(data.pitch,    0, 1023, 1000, 2000);
  ppm[2] = map(data.throttle, 0, 1023, 1000, 2000);
  ppm[3] = map(data.yaw,      0, 1023, 1000, 2000);
  ppm[4] = map(data.aux1,     0, 1, 1000, 2000);
  ppm[5] = 1000;
}
#endif

#ifdef USE_SBUS
void setSBUSValuesFromData()
{
  sbus.chan0 = map(data.roll,     0, 1023, 192, 1971);
  sbus.chan1 = map(data.pitch,    0, 1023, 192, 1971);
  sbus.chan2 = map(data.throttle, 0, 1023, 192, 1971);
  sbus.chan3 = map(data.yaw,      0, 1023, 192, 1811);
  sbus.chan4 = map(data.aux1,     0,    1, 192, 1811);
  sbus.chan5 = 172;
}
#endif
