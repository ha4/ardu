#include "iface_nrf24l01.h"

//#define DEBUG
//#define USE_PPM
#define USE_SBUS

/*
 SPI Comm.pins with nRF24L01
 */
#define MOSI_pin  11  
#define SCK_pin   13
#define CS_pin    10  
#define MISO_pin  12 
#define CE_pin    9

#define MOSI_on PORTB |= _BV(3)  
#define MOSI_off PORTB &= ~_BV(3)
#define SCK_on PORTB |= _BV(5)   
#define SCK_off PORTB &= ~_BV(5) 
#define CE_on PORTB |= _BV(1)   
#define CE_off PORTB &= ~_BV(1) 
#define CS_on PORTB |= _BV(2)   
#define CS_off PORTB &= ~_BV(2) 
// SPI input
#define  MISO_on (PINB & _BV(4)) 

#define RF_POWER TX_POWER_158mW 
//TX_POWER_5mW  80 20 158

struct MyData {
  uint16_t throttle;
  uint16_t yaw;
  uint16_t pitch;
  uint16_t roll;
  byte aux1;
};

MyData data;
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
uint32_t pps,ppscnt;

void setup()
{
    pinMode(MOSI_pin, OUTPUT);
    pinMode(SCK_pin, OUTPUT);
    pinMode(CS_pin, OUTPUT);
    pinMode(CE_pin, OUTPUT);
    pinMode(MISO_pin, INPUT);

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
    t1s=ref_t;
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
}

#ifdef USE_PPM
void setPPMValuesFromData()
{
  ppm[0] = map(data.roll,     0, 1023, 1000, 2000);  
  ppm[1] = map(data.pitch,    0, 1023, 1000, 2000);
  ppm[3] = map(data.yaw,      0, 1023, 1000, 2000);
  ppm[2] = map(data.throttle, 0, 1023, 1000, 2000);
  ppm[4] = map(data.aux1,     0, 1, 1000, 2000);
  ppm[5] = 1000;
}
#endif

#ifdef USE_SBUS
void setSBUSValuesFromData()
{
  sbus.chan0 = map(data.roll,     0, 1023, 172, 1811);
  sbus.chan1 = map(data.pitch,    0, 1023, 172, 1811);
  sbus.chan2 = map(data.yaw,      0, 1023, 172, 1811);
  sbus.chan3 = map(data.throttle, 0, 1023, 172, 1811);
  sbus.chan4 = map(data.aux1,     0,    1, 172, 1811);
  sbus.chan5 = 172;
}
#endif
