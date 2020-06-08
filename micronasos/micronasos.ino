enum {oDir = 4, oStp=5, oEn=3, i5=14, i9=15, tPulse=15};

void setup()
{
  pinMode(oDir, OUTPUT);  digitalWrite(oDir, 0);
  pinMode(oStp, OUTPUT);  digitalWrite(oStp, 0);
  pinMode(oEn,  OUTPUT);  digitalWrite(oEn, 0);
  pinMode(i5, INPUT);
  pinMode(i9, INPUT);
  Serial.begin(115200);
}

volatile int32_t cnt=0;
volatile int32_t spd=1000;
volatile int32_t dly=1000;
volatile int32_t acc=10000;
volatile bool  run=0;

void set_run(bool go)
{
  if(run==go)return;
  if (go) digitalWrite(oEn, 0);
  else digitalWrite(oEn, 1);
  delayMicroseconds(dly);
  Serial.println(go?"run":"stop");
  run = go;
}

void set_spd(int32_t speed_)
{
    spd = speed_;
    if (spd > 0) dly=1000000L/spd;
}

void parse_serial()
{
    switch(Serial.read()) {
      case 'l': cnt += Serial.parseInt(); break;
      case 'u': cnt += Serial.parseInt(); break;
      case 'r': cnt -= Serial.parseInt(); break;
      case 'd': cnt -= Serial.parseInt(); break;
      case 'n': cnt = Serial.parseInt(); break;
      case 's': set_spd(Serial.parseInt()); break;
    }
    Serial.print("i5/9:");
    Serial.print(digitalRead(i5));
    Serial.print(digitalRead(i9));
    Serial.print(" speed:");
    Serial.print(spd);
    Serial.print(" dly:");
    Serial.print(dly);
    Serial.print(" cnt:");
    Serial.println(cnt);
    if (cnt!=0)set_run(1);
}

void loop()
{
  if (Serial.available()) parse_serial();
  
  if (run) {
    delayMicroseconds(dly-tPulse);
    if (cnt < 0) {
      if (digitalRead(i9)==0) { set_run(0); return; }
      cnt++;
      digitalWrite(oDir, 0);
      digitalWrite(oStp, 1); delayMicroseconds(tPulse); digitalWrite(oStp, 0);
    } else if (cnt > 0) {
      if (digitalRead(i5)==0) { set_run(0); return; }
      cnt--;
      digitalWrite(oDir, 1);
      digitalWrite(oStp, 1); delayMicroseconds(tPulse); digitalWrite(oStp, 0);
    }
    if (cnt == 0) set_run(0);
  }
}

