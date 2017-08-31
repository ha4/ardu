// include the library code:
#include <LCDnum.h>

// initialize the library with the numbers of the interface pins
LCDnum panel(12, 11);

void setup() {
  panel.print("123,4");
}

void loop() {
  panel.noDisplay();
  delay(500);
  panel.display();
  delay(500);
}

