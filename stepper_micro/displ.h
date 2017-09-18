#include <LiquidCrystal.h>

extern LiquidCrystal lcd1;

void LCD_init();
int dial_print(int v);
void status_show(int run);
int menu_name(int p);
int menu_show(int p);
void displ_volt(float x);
void show_timer(int m, int s);



