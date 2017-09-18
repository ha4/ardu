#include <LiquidCrystal.h>

#include "displ.h"

enum { 
  disp_4=14, disp_5, disp_6=6, disp_7, disp_e=13, disp_rs=12,
};


LiquidCrystal lcd1(disp_rs, disp_e, disp_4, disp_5, disp_6, disp_7);

void LCD_init()
{
  lcd1.begin(8, 2, 0);
  lcd1.noDisplay();
  lcd1.noAutoscroll();
  lcd1.noCursor();
  lcd1.noBlink();
  lcd1.display();
  lcd1.print("OK");
}

int dial_print(int v)
{
  lcd1.setCursor(0,1); 
  lcd1.print(v);
  lcd1.print("  ");
  return v;
}

void status_show(int run)
{
  lcd1.setCursor(0,0);
  if (run)
    lcd1.print(F2(xname_strun));
  else
    lcd1.print(F2(xname_stok));
}

int menu_name(int p)
{
  if(p>=0  && p<=7) lcd1.print(F2(xname_menu));
  if(p>=20 && p<=26) lcd1.print(F2(xname_setup));
  if(p>=10 && p<=12) lcd1.print(F2(xname_move));
  if(p>=30 && p<=32) lcd1.print(F2(xname_save));
}

int menu_show(int p)
{
  lcd1.setCursor(0,1);
  switch(p){
  default:  p = 0;
  case 0:  lcd1.print(F2(xname_exit));  break;
  case 1:  lcd1.print(F2(xname_move));  break;
  case 2:  lcd1.print(F2(xname_dir));   break;
  case 3:  lcd1.print(F2(xname_spd));   break;
  case 4:  lcd1.print(F2(xname_dura));  break;
  case 5:  if(running) lcd1.print(F2(xname_stop));  else lcd1.print(F2(xname_start)); break;
  case 6:  lcd1.print(F2(xname_setup)); break;
  case 8:  p=7;
  case 7:  lcd1.print(F2(xname_ver)); break;
    
  case  9: p=10;
  case 10: lcd1.print(F2(xname_ccw));  break;
  case 11: lcd1.print(F2(xname_none)); break;
  case 13: p=13;
  case 12: lcd1.print(F2(xname_cw)); break;

  case 19: p=20;
  case 20: lcd1.print(F2(xname_exit)); break;
  case 21: lcd1.print(F2(xname_ustp)); break;
  case 22: lcd1.print(F2(xname_nstp)); break;
  case 23: lcd1.print(F2(xname_ipk)); break;
  case 24: lcd1.print(F2(xname_sens)); break;
  case 25: lcd1.print(F2(xname_revr)); break;
  case 27: p=26;
  case 26: lcd1.print(F2(xname_save)); break;

  case 29: p=30;
  case 30: lcd1.print(F2(xname_yes)); break;
  case 31: lcd1.print(F2(xname_no)); break;
  case 33: p=32;
  case 32: lcd1.print(F2(xname_dfl)); break;

  }
  return p;
}

void displ_volt(float x)
{
  lcd1.setCursor(3,0);
  if (x < 10) lcd1.print(' ');
  lcd1.print(x,1);
  lcd1.print('v');
}

void show_timer(int m, int s)
{
    lcd1.setCursor(0, 1);
    lcd1.print(m);
    lcd1.print(':');
    if (s < 10) lcd1.print('0');
    lcd1.print(s);
    lcd1.print(' ');
}
