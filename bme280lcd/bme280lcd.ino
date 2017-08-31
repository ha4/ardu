/***************************************************************************
  This is a library for the BME280 humidity, temperature & pressure sensor

  Designed specifically to work with the Adafruit BME280 Breakout
  ----> http://www.adafruit.com/products/2650

  These sensors use I2C or SPI to communicate, 2 or 4 pins are required
  to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
#define LCD_VO 6

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // I2C
//Adafruit_BME280 bme(BME_CS); // hardware SPI
//Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO,  BME_SCK);

void setup() {
  analogWrite(LCD_VO,1);
  Serial.begin(9600);
  lcd.begin(20, 2);
  Serial.println(F("BME280 test"));

  if (!bme.begin()) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
}

void loop() {
    float v;
    v = bme.readTemperature();
    Serial.print("Temperature = ");
    Serial.print(v);
    Serial.println(" *C");
    lcd.setCursor(0,0);
    lcd.print("T "); lcd.print(v); lcd.print(" \xEF""C  ");

//    v = bme.readPressure() / 100.0F;
//    Serial.print("Pressure = ");
//    Serial.print(v);
//    Serial.println(" hPa");

//    v = bme.readAltitude(SEALEVELPRESSURE_HPA);
//    Serial.print("Approx. Altitude = ");
//    Serial.print(v);
//    Serial.println(" m");

    v = bme.readHumidity();
    Serial.print("Humidity = ");
    Serial.print(v);
    Serial.println(" %");
    lcd.setCursor(0,1);
    lcd.print("RH "); lcd.print(v); lcd.print(" %  ");

    Serial.println();
    delay(2000);
}
