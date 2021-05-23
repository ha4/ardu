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
#include "symax_nrf24l01.h"
#include "xtimer.h"


enum { pinLED = 3 } ;

xtimer timer;
xtimer flsh;
SymaXData myData;

int sa=0, ca=511, sb=511, cb=0;
int  limadd(int a, int b) 
{
  a+=b;
  if(a >  511) a=511;
  if(a < -511) a=-511;
  return a;
}

void readtest()
{
    myData.aileron=sa/4;
    myData.elevator=cb/4;
    myData.throttle=(511+sb)>>2;
    myData.rudder=ca/4;

    int ts=sa/128,  tc=ca/128;
    sa=limadd(sa,tc), ca=limadd(ca,-ts);
    ts=sb/32,  tc=cb/32;
    sb=limadd(sb,tc), cb=limadd(cb,-ts);
}

void setup()
{
  Serial.begin(115200);
  
  pinMode(pinLED, OUTPUT);

  flsh.set(333);
  flsh.start(millis());
  timer.start(micros());
  symax_tx_id(0x7F7FC0D7ul);

  symax_data(&myData);
    myData.trims[0]=myData.trims[1]=myData.trims[0]=0;
    myData.flags4=myData.flags6=myData.flags7=0;
    myData.flags5=FLAG5_HIRATE;

  timer.set(symax_init());
}

void loop() 
{ 
  if (timer.check(micros()))  {
    timer.set(symax_callback());
    readtest();
  }
//  if (symax_binding()) { if (flsh.check(millis())) digitalWrite(pinLED, 1-digitalRead(pinLED)); }
//  else digitalWrite(pinLED,1);
}
