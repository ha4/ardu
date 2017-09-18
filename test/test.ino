/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */
 
// Pin 13 has an LED connected on most Arduino boards.
// give it a name:
// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
  Serial.begin(9600);
}

// the loop routine runs over and over again forever:
void loop() {
//  if (Serial.available()) Serial.write(Serial.read());
//  Serial.println("x5");
 if (Serial.available()) switch(Serial.read()){case'0':digitalWrite(13,0);break;case'1':digitalWrite(13,1);break;}
}
