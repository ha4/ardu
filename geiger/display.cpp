
#include "geiger.h"

#include <LCDnum.h>

LCDnum panel(DISPLAY_L_PIN, DISPLAY_BASE_PIN);

unsigned long volatile tlcd=0;
unsigned long volatile slcd=0;
bool olcd=1;
uint8_t blaank[4]={0,0,0,0};

void lcd_refresh()
{
  panel.refresh(0);
}


void lcd_float(float x)
{
  if(isnan(x)) return;
  if (x<9.9995)  panel.println(x,3);
  else if (x<99.995)  panel.println(x,2);
  else if (x<999.95) panel.println(x,1);
  else if (x<9999.5) panel.println(x,0);
  else { panel.print(x/10,1); panel.print('.'); }
}

void lcd_int(int x)
{
  panel.println(x);
}

void lcd_str(char* x)
{
  panel.println(x);
}

// 0bFADEGBCH
void lcd_msg(uint8_t *x)
{
  if (x) {
    olcd=0;
    panel.vwrite((uint8_t*)x,4);
  } else {
    olcd=1;
    panel.vwrite(blaank,4);
  }
}
