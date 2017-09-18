void setup() {
  Serial.begin(9600);
}

void loop() {
  int a0,a1;
  float v0,v1;
  a0 = analogRead(A0);
  a1 = analogRead(A1);
  v0=map(a0,0,1024,0,5000); // 5000mv
  v1=map(a1,0,1024,0,5000); // 5000mv
  Serial.print(v0);
  Serial.print(' ');
  Serial.print(v1);
  Serial.println();
}
