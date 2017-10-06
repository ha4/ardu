void setup()
{
  Serial.begin(57600);
  pinMode(3,OUTPUT); digitalWrite(3,1);
  pinMode(4,OUTPUT); digitalWrite(4,1);
  pinMode(5,OUTPUT); digitalWrite(5,1);
  pinMode(6,OUTPUT); digitalWrite(6,1);
}

void loop()
{
  int m;
  if (Serial.available()) switch(Serial.read()) {
    case 'a': m = Serial.parseInt(); analogWrite(3,m); break;
    case 'b': m = Serial.parseInt(); analogWrite(4,m); break;
    case 'c': m = Serial.parseInt(); analogWrite(5,m); break;
    case 'd': m = Serial.parseInt(); analogWrite(6,m); break;
  }
}

