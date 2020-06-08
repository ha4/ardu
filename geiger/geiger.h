#ifndef GEIGER_H
#define GEIGER_H

#include "Config.h"
#include "Arduino.h"

/* serial */
#ifdef USE_SERIAL
#include "HardwareSerial.h"
#define MYSERIAL Serial
#endif                 

/* display */
extern unsigned long volatile tlcd;
extern unsigned long volatile slcd;
extern bool olcd;
extern void lcd_refresh();
extern void lcd_float(float x);
extern void lcd_int(int x);
extern void lcd_str(char* x);
extern void lcd_msg(uint8_t *x);


/* eeprom pointers */
#define PARNUM sizeof(parc)
#define PARSAVE 1 /* first 5 save to eeprom */
#define PARSIZE 2 /* element size */

extern char  parc[]; // {  'u', 'P', 'I', 'D' };
extern void* parv[]; // {  &setp, &Pb, &Ti, &Td };

#define EEPROM_START 2
#define EEPROM_MAX 126
extern int crcEEprom();
extern void wpar();
extern int lpar();
extern void clrpar();
extern void par_show();

/* u-converter */
extern uint16_t  volatile setp;
extern float kp, ki;
extern float volatile u, ui, mv, ub;
extern void ubatt();
extern void ustab();
extern void ustab_show();

/* counter */
extern byte volatile ir;
extern unsigned long volatile tcpm;
extern uint32_t volatile i_pulse;
extern float volatile cpmfactor;
extern int volatile use_rcpm;
extern int volatile use_cpm;
extern void gm_intervals();
extern void gm_counter();
extern void gm_pulse();
extern void gm_show();
extern float filter(float y, float x, float a);

/* main */
extern void counter_value(float n);
extern void counter_int(uint16_t n);
extern void powerdown();
extern void menu_popup(char *x);
extern void menu_level();
extern void menu_action(uint8_t a);
extern void menu_next();
extern void menu_mode(uint8_t m);
extern void menu_cmd(byte c);

#ifdef DEBUG_ENABLE
#define DEBUG(x) MYSERIAL.print(x)
#else
#define DEBUG(x)
#endif

#endif
