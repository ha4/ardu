#include "eeparam.h"
#include "displ.h"
#include "encoder.h"
#include "stepper.h"
/*
 *   constructor for four-pin version
 *   Sets which wires should control the motor.
 *
 *  *-USED  X-BROKEN 0-USEDasGND
 
                      *     *    0    *   d5*   d4*                rs*
 /PD1 /PD0      /PC5 /PC4 /PC3 /PC2 /PC1 /PC0                /PB5-LED
 /PD2 *TX  *RX   *    D19  D18  D17  D16  D15  D14                /SCK
 D2   D1   D0  RST   A5   A4   A3   A2   A1   A0   A7  AREF  A6  D13
 ---------------------------------------------------------------------
 |(28) (27) (26) (25) (24) (23) (22) (21) (20) (19) (18) (17) (16) (15)|
 | #32  #31  #30  #29  #28  #27  #26  #25  #24  #23  #22  #20  #19 #17 |
 |\         *RESET             /mega \               <LED-D13>     |USB-|
 |/     ((16MHz))          #18 \ 168 /                             |PORT|
 | #1  #2   #3  78L05  #3   #4   #9  #10  #11  #12  #13  #14  #15  #16 |
 |(01) (02) (03) (04) (05) (06) (07) (08) (09) (10) (11) (12) (13) (14)|
 ---------------------------------------------------------------------
 D3   D4  GND  Vin   GND  Vcc  D5   D6   D7   D8   D9  D10  D11  D12
 /PD3 /PD4                     /PD5 /PD6 /PD7 /PB0 /PB1 /SS /MOSI /MISO
 /PB2 /PB4 /PB4 
 *    *    *          *    *    X  d6*   d7*   *    *    *    *   e*
 
 motor: 3,4,8,9,10,11
 encoder: 16,17,18,19
 serial: 0,1,rst
 display: 14,15,6,7,12,13
 broken: 5
 analogsens: a6
 stopper:2
 free:  a7
 
 */

#define F2(string) (reinterpret_cast<const __FlashStringHelper *>(string))

const prog_uchar sw_version[]  PROGMEM = "v1.01"; // 1,2,4,8,16

const prog_uchar xname_ustp[]  PROGMEM = "uStep   "; // 1,2,4,8,16
const prog_uchar xname_nstp[]  PROGMEM = "StepN   "; // 200
const prog_uchar xname_ipk[]   PROGMEM = "Ipk     ";   // 1800 mA
const prog_uchar xname_sens[]  PROGMEM = "Gsens   "; // 2 sm
const prog_uchar xname_revr[]  PROGMEM = "Revers  ";// 0/1
const prog_uchar xname_dir[]   PROGMEM = "Direction";   // 1/-1
const prog_uchar xname_spd[]   PROGMEM = "Speed   "; // 3600 rph
const prog_uchar xname_run[]   PROGMEM = "Run     ";   // 0/1
const prog_uchar xname_dura[]  PROGMEM = "Duration";  // 1 min

const prog_uchar xname_menu[]  PROGMEM = "MENU"; 
const prog_uchar xname_exit[]  PROGMEM = "Exit    ";
const prog_uchar xname_setup[] PROGMEM = "SETUP   ";
const prog_uchar xname_ver[]   PROGMEM = "Version ";
const prog_uchar xname_save[]  PROGMEM = "Save?   ";
const prog_uchar xname_stop[]  PROGMEM = "Stop    ";
const prog_uchar xname_start[] PROGMEM = "Start   ";
const prog_uchar xname_move[]  PROGMEM = "Move    ";
const prog_uchar xname_none[]  PROGMEM = "None";
const prog_uchar xname_cw[]    PROGMEM = "CW  ";
const prog_uchar xname_ccw[]   PROGMEM = "CCW ";
const prog_uchar xname_yes[]   PROGMEM = "  YES  ";
const prog_uchar xname_no[]    PROGMEM = "   NO  ";
const prog_uchar xname_dfl[]   PROGMEM = "DEFAULT";

const prog_uchar xname_strun[]   PROGMEM = "RUN"; 
const prog_uchar xname_stok[]    PROGMEM = "OK "; 

const __FlashStringHelper* parn[] = { 
  F2(xname_ustp), F2(xname_nstp), F2(xname_ipk),  F2(xname_sens),
  F2(xname_revr), F2(xname_dir),  F2(xname_spd),  F2(xname_dura), F2(xname_run) };

// typedef int8_t PROGMEM prog_int8_t;

const char  parc[] = {  
  'u', 'N', 'I', 'G',
  'r', 'D', 'S', 'T', 'R'};
void* parv[] =       {  
  &STP_usteps, &STP_npole, &STP_mA, &STP_Gsens,
  &STP_reverse, &STP_dir, &STP_speed, &run_amount, &running};


int limit(int v, int m, int M)
{
  if (v < m) return m;
  if (v > M) return M;
  return v;
}

int dial(const __FlashStringHelper* name, int v0, int m, int M)
{
  int v = v0;
  lcd1.clear();
  lcd1.setCursor(0,0); 
  lcd1.print(name);

  v = dial_print(limit (v,m,M));

  for(;;) {
    switch(inputRot())
    {
    case ENC_CW: 
      v = dial_print(limit (v+1,m,M));   
      break;
    case ENC_CCW: 
      v = dial_print(limit (v-1,m,M));   
      break;
    }
    switch(inputBttn()){
    case ENC_CLICK:       
      return v;
    case ENC_LONG:       
      return v0; 
    }
  }
  return v;
}

volatile int menu_in, menu_p;

int menu_click()
{
  int x;
  Serial.print("click");
  Serial.println(menu_p);
  
  switch(menu_p) {
  default:
  case 0: /* exit */  menu_in = 0;  return 0;
  case 1: /* move */  menu_p = 11; return 1;
  case 2: /* move */  STP_dir = dial(F2(xname_dir),STP_dir,-1,1);  return 1;
  case 3: /* move */  x = dial(F2(xname_spd),STP_speed,0,32000);  setSpeed(x, STP_npole, STP_usteps);  return 1;
  case 4:  run_amount = dial(F2(xname_dura),run_amount,1,32000);   return 1;
  case 5:
    if(running)
      tick_stop();
    else
      tick_start();
    status_show(running);
    menu_p=0;
    return 0;
  case 6:  menu_p = 20; return 1;
  case 7:  dial(F2(sw_version),0,0,0); return 1;
    
  case 10: /* move */
  case 11: /* move */
  case 12: /* move */  menu_p = 1; return 1;

  case 20: /* exit */  menu_p = 6; return 1;
  case 21: /* uStep */ x=dial(F2(xname_ustp),STP_usteps,1,16); setSpeed(STP_speed, STP_npole, x); return 1;
  case 22: /* steps */ x=dial(F2(xname_nstp),STP_npole,1,800); setSpeed(STP_speed, x, STP_usteps); return 1;
  case 23: /* Ipk */   x=dial(F2(xname_ipk),STP_mA,1,3000); setCurrent(x, STP_Gsens); return 1;
  case 24: /* Gsens */ x=dial(F2(xname_sens),STP_Gsens,1,20); setCurrent(STP_mA, x);return 1;
  case 25: /* Revers */ STP_reverse=dial(F2(xname_revr),STP_reverse,0,1); return 1;
  case 26: /* Save */  menu_p = 31; return 1;

  case 32: /* defaults */ clrpar(); menu_p = 0; return 0;
  case 30: /* yes */ wpar();  menu_p = 0; return 0;
  case 31: /* NO */
    menu_p = 26; return 1;
  }
}

void menu(int act)
{
  if (menu_in) {
    if (menu_p==10) { rawStepper(-1); delayMicroseconds(500); }
    if (menu_p==12) { rawStepper(1); delayMicroseconds(500); }
  }
  if (act==ENC_NULL) return;
  switch(act)  {
  case ENC_LONG:
    if (menu_in) {
      menu_in=0;
      menu_p=0; 
      lcd1.clear();
    } else { // not menu, cancel run
      tick_stop();
      status_show(running);
    }
    break;
  case ENC_CLICK: 
    if (!menu_in) {
      menu_in = 1;
      menu_p = 0;
    menu_activate:
      lcd1.clear();
      menu_name(menu_p);
      menu_p = menu_show(menu_p);
    } else { // menu_in=1
      lcd1.clear();
      if(menu_click()) goto menu_activate;
      menu_in=0;
    }
    break;
  case ENC_CW:     menu_p = menu_show(menu_p+1);    break;
  case ENC_CCW:    menu_p = menu_show(menu_p-1);    break;
  }
}

#define VOLTMETER_PERIOD 1000
long volt_timr;
enum {   
  volt_pin=A6 };

void voltmeter_init()
{
  analogReference(INTERNAL);
  analogRead(volt_pin);
  volt_timr = millis()-VOLTMETER_PERIOD;
}

void voltmeter()
{
  long now = millis();

  if (now - volt_timr >= VOLTMETER_PERIOD) {
    volt_timr=now;
    float x;
    x = 15.58*1.100/1024.*analogRead(volt_pin); // Kdiv*Uref/Precision*Code
    displ_volt(x);
  }
}

unsigned int tick_countdown;
int tick_sec;
int tick_amount;
long tick_tmr;

enum {tick_stopper = 2 };

void tick_init()
{
  pinMode(tick_stopper, INPUT_PULLUP);
  tick_amount = 0;
  tick_sec = 0;
}

void tick_start()
{
  tick_sec = 1;
  tick_amount = run_amount;
  tick_tmr=millis()-1000; // 1 second
  running = 1;
}

void tick_timer()
{
  long now = millis();

  if (now - tick_tmr >= 1000) { // 1 second
    tick_tmr=now;
    if (!running) return;
    if (--tick_sec < 0) {
      tick_sec+=60;
      if (--tick_amount < 0) { 
        tick_stop();
        status_show(running);
      }
    }
    show_timer(tick_amount, tick_sec);
  }
}

void tick_stop()
{
  tick_sec = tick_amount = 0;
  running=0;
}


void setup() {
  Serial.begin(57600);

  LCD_init();
  voltmeter_init();
  Stepper_init();
  enc_init();
  tick_init();
  menu_in=0;

  lpar();
  setSpeed(STP_speed, STP_npole, STP_usteps);
  setCurrent(STP_mA, STP_Gsens);
  running = 0;
}


void loop() {
  if(running) pulseStepper();
  menu(inputBttn());
  if (menu_in)
    menu(inputRot());
  else {
    voltmeter();
    tick_timer();
  }
}












