/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

/**
 * Channel scanner
 *
 * Example to detect interference on the various channels available.
 * This is a good diagnostic tool to check whether you're picking a
 * good channel for your application.
 *
 * Inspired by cpixip.
 * See http://arduino.cc/forum/index.php/topic,54795.0.html
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

#define CE_PIN 9
#define nCS_PIN 10

RF24 radio(CE_PIN, nCS_PIN);

const uint8_t num_channels = 128;
uint8_t values[num_channels];

void setup(void)
{
  Serial.begin(115200);
  Serial.println(F("\n\rRF24/examples/scanner/"));

  radio.begin();
  radio.setAutoAck(false);

  // Get into standby mode
  radio.startListening();
  radio.stopListening();

  // Print out header, high then low digit
  
  for(int i = 0; i < num_channels; i++) Serial.print(i>>4,HEX);
  Serial.println();
  for(int i = 0; i < num_channels; i++) Serial.print(i&0xf,HEX);
  Serial.println();
}

const int num_reps = 25;

void loop(void)
{
  // Clear measurement values
  memset(values,0,sizeof(values));

  // Scan all channels num_reps times
  for(int rep_counter = num_reps; rep_counter--;) {
  for (int i = num_channels;i--;) {
      radio.setChannel(i);
      radio.startListening();
      delayMicroseconds(1000);
      if (radio.testCarrier()) ++values[i];
      radio.stopListening();
  }
  }

  for(int i = 0; i < num_channels; i++)
     Serial.print(min(0xf,values[i]&0xf),HEX);
  Serial.println();
}

// vim:ai:cin:sts=2 sw=2 ft=cpp
