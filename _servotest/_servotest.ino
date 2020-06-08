#include <Servo.h>

Servo myServo;  // create a servo object 

void setup() {
  myServo.attach(11,1000,2000); // attaches the servo on pin 9 to the servo object 
  Serial.begin(9600); // open a serial connection to your computer
}

int volatile angle = 0;
int volatile sangle = 0;

void loop() {
  while (Serial.available()) {
    bool sx=0;
    if(Serial.peek()=='s') {Serial.read(); sx=1;}
    sangle = Serial.parseInt();
    Serial.print("set ");
    Serial.println(sangle);
    if(sx)angle=sangle;
    if(Serial.available() && Serial.read()=='\n') break;
    
  }
  if (angle != sangle) {
    if (angle > sangle) angle--; else angle++;
    Serial.println(angle);
  }
  // set the servo position  
  myServo.write(angle);

  // wait for the servo to get there 
  delay(15);
}


