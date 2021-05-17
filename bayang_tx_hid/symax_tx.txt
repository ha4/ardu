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
#include "interface.h"
#include "xtimer.h"


enum { pinLED = 3 } ;

xtimer timer;
xtimer flsh;

void setup()
{
  Serial.begin(115200);
  
  pinMode(pinLED, OUTPUT);

  flsh.set(333);
  flsh.start(millis());
  timer.start(micros());
  timer.set(0);
}

void loop() 
{ 
  if (timer.check(micros()))  timer.set(symax_callback());
  if (symax_binding()) { if (flsh.check(millis())) digitalWrite(pinLED, 1-digitalRead(pinLED)); }
  else digitalWrite(pinLED,1);
}

