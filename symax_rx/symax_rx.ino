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

enum { pinLED = 18 } ;

xtimer timer;
xtimer flsh;


void setup()
{
  Serial.begin(115200);
  
  pinMode(pinLED,OUTPUT);

  flsh.set(500);
  flsh.start(millis());
  timer.set(0);
  timer.start(micros());
}

void loop() 
{ 
  int values[8];
  if (timer.check(micros()))  timer.set(symax_run(values));
  if (symax_binding()) { if (flsh.check(millis())) digitalWrite(pinLED, 1-digitalRead(pinLED)); }
  else {
      digitalWrite(pinLED,1);
      Serial.print("T"); Serial.print(values[0]);
      Serial.print(" R");  Serial.print(values[1]);
      Serial.print(" E");Serial.print(values[2]);
      Serial.print(" A"); Serial.print(values[3]);
      Serial.print(" F");     Serial.print(values[4]);
      Serial.println();
  }
  }


