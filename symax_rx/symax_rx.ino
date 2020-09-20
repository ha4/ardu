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
#include "interface.h"
#include "xtimer.h"

#define USE_SERIALSTRING
//#define USE_PPM
//#define USE_SBUS

enum { pinLED = 18, pinPPM = 5, pinSync= 4} ;

xtimer timer;
xtimer flsh;

/*
 * PPM part
 */
#define PPM_number 8  //set the number of chanels
#define PPM_micros 2  // timer resolution scale factor 16Mhz==2
#define default_servo_value 1500  //set the default servo value
#define PPM_PulseLen 300  //set the pulse length
#define PPM_FrLen (22500-PPM_PulseLen)  //set the PPM frame length in microseconds (1ms = 1000µs)
#define PPM_onState 1  //set polarity of the pulses: 1 is positive, 0 is negative

int ppm[PPM_number];
boolean PPM_phase,PPM_state;
byte PPM_chan;
unsigned int PPM_rest;

void ppm_start() {
  for(int i=0; i<PPM_number; i++)
    ppm[i]= default_servo_value;

  pinMode(pinPPM, OUTPUT);
  pinMode(pinSync, OUTPUT);
  digitalWrite(pinPPM, !PPM_onState);  //set the PPM signal pin to the default state (off)
  
  cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 100;  // compare match register, change this
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  PPM_phase = true;
  PPM_chan = 0;
  PPM_rest = PPM_FrLen;
  sei();
  PPM_state = true;
}

void ppm_stop() {
  cli();
  TCCR1A = 0;
  TCCR1B = 0; // no timer clocking
  TIMSK1 &= ~(1 << OCIE1A);
  sei();
  PPM_state = false;
  digitalWrite(pinPPM, !PPM_onState);
}

ISR(TIMER1_COMPA_vect)
{
  TCNT1 = 0;
  
  if (PPM_phase) {  //start pulse
    digitalWrite(pinPPM, PPM_onState);
    OCR1A = PPM_PulseLen * PPM_micros;
    PPM_phase = false;
    return;
  }
  digitalWrite(pinPPM, !PPM_onState);

  if(PPM_chan < PPM_number){
    OCR1A = (ppm[PPM_chan] - PPM_PulseLen) * PPM_micros;
    PPM_rest -= ppm[PPM_chan];
    PPM_chan++;
  } else {
    OCR1A = PPM_rest * PPM_micros;
    PPM_rest = PPM_FrLen;
    PPM_chan = 0;  
  }
  PPM_phase = true;
}

/*
 * SBUS part
 */
#define SBUS_FLAG_SIGNAL_LOSS       (1 << 2)
#define SBUS_FLAG_FAILSAFE_ACTIVE   (1 << 3)

typedef struct sbusChannels_s {
    // 176 bits of data (11 bits per channel * 16 channels) = 22 bytes.
    unsigned int chan0 : 11;
    unsigned int chan1 : 11;
    unsigned int chan2 : 11;
    unsigned int chan3 : 11;
    unsigned int chan4 : 11;
    unsigned int chan5 : 11;
    unsigned int chan6 : 11;
    unsigned int chan7 : 11;
    unsigned int chan8 : 11;
    unsigned int chan9 : 11;
    unsigned int chan10 : 11;
    unsigned int chan11 : 11;
    unsigned int chan12 : 11;
    unsigned int chan13 : 11;
    unsigned int chan14 : 11;
    unsigned int chan15 : 11;
    uint8_t flags;
} __attribute__((__packed__)) sbusChannels_t;

 sbusChannels_t sbus;

void sbus_start() 
{
  Serial.begin(100000,SERIAL_8E2);
  memset(&sbus,0,sizeof(sbus));
}

void sbus_send() {
  Serial.write("\x0F",1);
  Serial.write((uint8_t*)&sbus,sizeof(sbus));
  Serial.write("\x00",1);
}

void setup()
{
#ifdef USE_SERIALSTRING
  Serial.begin(115200);
  while(!Serial);
  Serial.println("timing Throttle Rudder Elevation Aileron Flags");
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
#endif
}

unsigned int byte2rc(unsigned int b)
{ /* 0..255 -> 1000..2000 scale=125/32 */
  return 1000+((125*b)>>5); 
}

unsigned int byte2sbus(unsigned int b)
{ /* 0..255 -> 192..1792 scale=25/4 */
  return 192+((25*b)>>2); 
}
/* AETR1234 */
uint32_t yy;

void loop() 
{ 
  int values[8];
  int ppmvalues[8];
  int rc;
  uint32_t dy,t;
  t=micros();
  if (timer.check(t))  timer.set(symax_run(values));

  rc=symax_binding();
  if (rc) {
    if (rc > 0) {
      if (flsh.check(millis())) digitalWrite(pinLED, 1-digitalRead(pinLED));
#ifdef USE_PPM
      if (PPM_state) ppm_stop();
#endif
    }
    return;
  }
    digitalWrite(pinLED,1);

#ifdef USE_SBUS
  /* convert TREAF -> AETR1234 */
  sbus.chan0=byte2sbus(128-values[3]);
  sbus.chan1=byte2sbus(128+values[2]);
  sbus.chan2=byte2sbus(values[0]);
  sbus.chan3=byte2sbus(128-values[1]);
  sbus.chan4=(values[4]&FLAG_LOW)?192:1792;
  sbus.chan5=(values[4]&FLAG_FLIP)?1792:192;
  sbus.chan6=(values[4]&FLAG_VIDEO)?1792:192;
  sbus.chan7=(values[4]&FLAG_PICTURE)?1792:192;
  sbus_send();
#endif

#ifdef USE_PPM
  /* convert TREAF -> AETR1234 */
  ppmvalues[0]=byte2rc(128-values[3]);
  ppmvalues[1]=byte2rc(128+values[2]);
  ppmvalues[2]=byte2rc(values[0]);
  ppmvalues[3]=byte2rc(128-values[1]);
  ppmvalues[4]=(values[4]&FLAG_LOW)?1000:2000;
  ppmvalues[5]=(values[4]&FLAG_FLIP)?2000:1000;
  ppmvalues[6]=(values[4]&FLAG_VIDEO)?2000:1000;
  ppmvalues[7]=(values[4]&FLAG_PICTURE)?2000:1000;
  
  ATOMIC_BLOCK(ATOMIC_FORCEON)
    { for(int i=0;i<8;i++) ppm[i]=ppmvalues[i]; }

  if (!PPM_state) ppm_start();
  //for(int i=0;i<8;i++) {Serial.print(ppmvalues[i]); Serial.print(' ');}
#endif

#ifdef USE_SERIALSTRING
  dy=t-yy;
  yy=t;
    Serial.print(dy);
    Serial.print(' ');  
  for(int i=0;i<5;i++) {
    Serial.print(values[i]);
    Serial.print(' ');
    }
  Serial.println();
#endif
}
