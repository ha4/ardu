
uint32_t cnt;

void setup()
{
  // start serial port at 9600 bps and wait for port to open:
  Serial1.begin(9600);
  while (!Serial1) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  
Serial1.println("Start");
}

void loop()
{
  uint32_t t;

  if (Serial1.available()) {
    Serial1.print("got:");
    while (Serial1.available() > 0) 
      Serial1.print((char)Serial1.read());
  }
  t=millis();
  if (t-cnt >= 500) { cnt=t; Serial1.print("rnd"); Serial1.println(random(10));}
}


