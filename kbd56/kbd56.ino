#include "keyboard.h"

void setup()
{
  Serial.begin(115200);
  keyboard_init();
}

void loop()
{
  keyboard_task();
}


