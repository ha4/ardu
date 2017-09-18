#include <IRremote.h>
#include <GRBlib.h>
#include <PinChangeInt.h>
// pwm pins: 3 5 6 9 10 11

IRrecv irrecv(2);
decode_results results;

void irisr()
{
	// Read if IR Receiver -> SPACE [xmt LED off] or a MARK [xmt LED on]
	uint8_t irdata = digitalRead(irparams.recvpin);
	unsigned int t = micros();

	// irparams.timer++;  // One more 50uS tick
	if (irparams.rawlen >= RAWBUF)  irparams.rcvstate = STATE_OVERFLOW ;

	switch(irparams.rcvstate) {
	case STATE_IDLE: // In the middle of a gap
		if (irdata == MARK) {
			if ((t-irparams.timer) >= _GAP) { // Gap just ended; Record duration; Start recording transmission
				irparams.overflow = false;
				irparams.rawlen = 0;
				irparams.rawbuf[irparams.rawlen++] = (t-irparams.timer)/50;
				irparams.rcvstate= STATE_MARK;
        		}
			irparams.timer = t;
		}
		break;
	case STATE_MARK:
		if (irdata == SPACE) {   // Mark ended; Record time
			irparams.rawbuf[irparams.rawlen++] = (t-irparams.timer)/50;
			irparams.timer = t;
			irparams.rcvstate = STATE_SPACE;
		}
		break;

	case STATE_SPACE:  // Timing Space
		if (irdata == MARK) {  // Space just ended; Record time
			irparams.rawbuf[irparams.rawlen++] = (t-irparams.timer)/50;
			irparams.timer                     = t;
			irparams.rcvstate                  = STATE_MARK;
                } else if ((t-irparams.timer) > _GAP) {  // Space
				// A long Space, indicates gap between codes
				// Flag the current code as ready for processing
				// Switch to STOP
				// Don't reset timer; keep counting Space width
				irparams.rcvstate = STATE_STOP;
		}
		break;
	case STATE_STOP:  // Waiting; Measuring Gap
		 if (irdata == MARK)  irparams.timer = t ;  // Reset gap timer
		 break;
	case STATE_OVERFLOW:  // Flag up a read overflow; Stop the State Machine
		irparams.overflow = true;
		irparams.rcvstate = STATE_STOP;
		break;
	}
}

void enable_irrecv(int pin)
{
	irparams.recvpin = pin;
	irparams.blinkflag = 0;
	irparams.rcvstate = STATE_IDLE;
	irparams.rawlen = 0;
        irparams.timer = micros();
        pinMode(pin, INPUT_PULLUP);
}

void setup()
{

  Serial.begin(9600);
  //irrecv.enableIRIn();
  enable_irrecv(2);
  attachPinChangeInterrupt(irparams.recvpin, irisr, CHANGE);
    Serial.println(F("---------"));
 
}

void printdecode()
{
    Serial.println(F("---------"));
    Serial.println(results.decode_type);
    Serial.println(results.address);
    Serial.println(results.value, HEX);
    Serial.println(results.bits);
    for (int i=0;i<results.rawlen;i++) { Serial.print(results.rawbuf[i], HEX);
    Serial.print(((i+1)==results.rawlen)?'\n':' '); }
    Serial.println(results.rawlen);
    Serial.println(results.overflow);
}

void loop() {
  if (irrecv.decode(&results)) {
    printdecode();
    irrecv.resume(); // Receive the next value
  }
  delay(50);
  irisr();
}

