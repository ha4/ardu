#ifndef __BTTNX__
#define __BTTNX__

enum {BTN_NONE=0, BTN_PRESS=1, BTN_RELEASE, BTN_CLICK, BTN_LONG, BTN_DCLICK, BTN_MCLICK };

// parameter in 'ms'
#define BTN_PERIOD 20
#define BTN_DEBOUNCE 50
#define BTN_DC 250
#define BTN_LC 3000

class button {
  public:
  button(int pin);
  byte poll();
  byte debounce();
  byte stat, prev;
  int8_t flt;
  byte cnt,cc;
  int p;
};

#endif

