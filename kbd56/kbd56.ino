#include <stdint.h>
#include <stdbool.h>
#include "keyboard.h"

void setup()
{
  Serial.begin(115200);
  keyboard_setup();
  keyboard_init();
          Serial.print("INIT COMPLETE");

}

void loop()
{
  keyboard_task();
}


