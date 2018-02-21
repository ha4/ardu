
enum { hx711_dt = 15, hx711_clk=14 };

void setup()
{
  Serial.begin(115200);
  pinMode(hx711_dt, INPUT_PULLUP);
  pinMode(hx711_clk, OUTPUT);
  digitalWrite(hx711_clk, 0);
}

void loop()
{
  if (digitalRead(hx711_dt)==0) {
    long v=0;
    for(int i=0; i < 24;i++) {
      digitalWrite(hx711_clk, 1);
      digitalWrite(hx711_clk, 0);
      v = (v<<1) | digitalRead(hx711_dt);
    }   
    for(int i=0; i < 1; i++) { // 1:A,128 2:B,32 3:A,64
      digitalWrite(hx711_clk, 1);
      digitalWrite(hx711_clk, 0);
    }
    if (v & 0x800000) v|=0xFF000000;
    Serial.println(v);
  }
}

