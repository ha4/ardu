#include <easySPI.h>

// initialize the library with the numbers of the interface pins
easySPI chan(SPI_MODE0 + SPI_CLOCK_DIV64);

ISR(TIMER1_OVF_vect) // 16MHz/65536 = 244Hz
{  panel.refresh(0); }

void setup() {
	cli();
	TCCR1A = 0;
	TCCR1B = 0;
	TIMSK1 = (1 << TOIE1);
	TCCR1B |= (1 << CS10);
	sei();
  panel.print("123,4");
}

void loop() {
  // Turn off the display:
  panel.noDisplay();
  delay(500);
   // Turn on the display:
  panel.display();
  delay(500);
}

