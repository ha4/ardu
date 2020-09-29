#include "iface_nrf24l01.h"

#define DEBUG

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

/* 
 PPM CONFIGURATION
*/
#define channel_number 6  //set the number of channels
#define PPM_pin 2  //set PPM signal output pin on the arduino
#define PPM_FrLen 27000  //set the PPM frame length in microseconds (1ms = 1000µs)
#define PPM_PulseLen 400  //set the pulse length
#define PPM_on PORTD |= _BV(2)
#define PPM_off PORTD &= ~_BV(2) 

int ppm[channel_number];

struct MyData {
  uint16_t throttle;
  uint16_t yaw;
  uint16_t pitch;
  uint16_t roll;
  byte aux1;
};

MyData data;

uint32_t ref_t;
uint32_t timout;

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

//    setupPPM();

    BAYANG_RX_data(&data);
    timout=BAYANG_RX_init();
    ref_t=micros();
}

void loop()
{
    uint32_t t=micros();

    if(t-ref_t >= timout) {
      ref_t = t;
      timout=BAYANG_RX_callback();
    }
    if(BAYANG_RX_available()) {
      Serial.print(data.roll);Serial.print(' ');
      Serial.print(data.pitch);Serial.print(' ');
      Serial.print(data.throttle);Serial.print(' ');
      Serial.print(data.yaw);Serial.println();
      
      // setPPMValuesFromData();
    }
}

void setPPMValuesFromData()
{
  ppm[0] = map(data.roll,     0, 1023, 1000, 2000);  
  ppm[1] = map(data.pitch,    0, 1023, 1000, 2000);
  ppm[3] = map(data.yaw,      0, 1023, 1000, 2000);
  ppm[2] = map(data.throttle, 0, 1023, 1000, 2000);
  ppm[4] = map(data.aux1,     0, 1, 1000, 2000);
  ppm[5] = 1000;
}

void setupPPM() {
  pinMode(PPM_pin, OUTPUT);
  PPM_off;

  cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;

  OCR1A = 100;  // compare match register (not very important, sets the timeout for the first interrupt)
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei();
}

//#error This line is here to intentionally cause a compile error. Please make sure you set clockMultiplier below as appropriate, then delete this line.
#define clockMultiplier 2 // set this to 2 if you are using a 16MHz arduino, leave as 1 for an 8MHz arduino

ISR(TIMER1_COMPA_vect){
  static boolean pulse = true;
  static byte n=0;
  static unsigned int rest=PPM_FrLen;

  TCNT1 = 0;

  if(pulse) { //end pulse
    PPM_off;
    OCR1A = PPM_PulseLen * clockMultiplier;
    pulse = false;
    return;
  }

  PPM_on;
  pulse = true;

  if(n >= channel_number) {
      rest -= PPM_PulseLen;
      OCR1A = rest * clockMultiplier;
      rest = PPM_FrLen;
      n=0;
  } else {
      rest -= ppm[n];
      OCR1A = (ppm[n] - PPM_PulseLen) * clockMultiplier;
      n++;
  }     
}
