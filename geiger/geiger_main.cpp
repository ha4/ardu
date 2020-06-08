
#include "geiger.h"
#include "buttn.h"

#ifdef USE_POWERBUTTON
button btn(POWER_BUTTON);
unsigned long btc=0;
#endif


void powerdown()
{
#ifdef USE_CONVERTER
  btc=millis();
  u=0;
  while(millis()-btc < 100) ustab();
#endif
#ifdef USE_POWERBUTTON
  digitalWrite(POWER_PIN, 0);
#endif
  for(;;);
}

// 0bFADE GBCH
uint8_t msg_cpm[] ={0b00111000, 0b11011100, 0b01011010, 0}; // c:deg p:abefg m:aceg
uint8_t msg_uSv[] ={0b10011100, 0b11101010, 0b10110010, 0}; // u:befg s:acdfg v:cdef
uint8_t msg_rcpm[]={0b00011000, 0b00111000, 0b11011100, 0b01011010}; // r:eg c:deg p:abefg m: aceg
uint8_t msg_icpm[]={0b00000010, 0b00111000, 0b11011100, 0b01011010}; // i:c c:deg p:abefg m: aceg
uint8_t msg_u[]   ={0b00110010, 0,0,0}; // u:cde
uint8_t msg_fact[]={0b11011000, 0b11011110, 0b00111000, 0b10111000}; // f:aefg a:abcefg c:deg t:defg
uint8_t msg_save[]={0b11101010, 0b11011110, 0b10110010, 0b11111000}; // s:acdfg a:abcefg v:cdef e:adefg
uint8_t msg_quit[]={0b11001110, 0b00110010, 0b00010000, 0b10111000}; // q:abcfg u:cde i:e t:defg

#define MENU_ITEMS 6
uint8_t menu_mdf=0, menu_idx=0, menu_dir=0, menu_poptime=0;
void menu_popup(uint8_t *x)
{
  lcd_msg(x);
  menu_poptime=DISPLAY_POPUP;
}

void menu_level()
{
  if (menu_mdf==1) switch(menu_idx) {
    case 0: lcd_msg(msg_cpm); break;
    case 1: lcd_msg(msg_rcpm); break;
    case 2: lcd_msg(msg_u); break;
    case 3: lcd_msg(msg_fact); break;
    case 4: lcd_msg(msg_save); break;
    case 5: lcd_msg(msg_quit); break;
  } else if (menu_mdf==2) switch(menu_idx) {
    case 0: lcd_msg(use_cpm?msg_cpm:msg_uSv); break;
    case 1: lcd_msg(msg_rcpm?msg_rcpm:msg_icpm); break;
    case 2: lcd_float(u*CONVERTER_MULT); break;
    case 3: lcd_float(cpmfactor); break;
    case 4: wpar(); menu_mode(0); break;
    case 5: menu_mode(0); break;
  }
}
void menu_action(uint8_t a)
{
  if (a==2) menu_dir=!menu_dir;
  else if (a==1) switch(menu_idx) {
    case 0: use_cpm = use_cpm? 0:1; lcd_msg(use_cpm?msg_cpm:msg_uSv); break;
    case 1: use_rcpm = use_rcpm? 0:1; lcd_msg(use_rcpm?msg_rcpm:msg_icpm); break;
    case 2: if (menu_dir) setp--; else setp++; lcd_float(u*CONVERTER_MULT); break;
    case 3: if (menu_dir) cpmfactor-=0.00001; else cpmfactor+=0.00001; lcd_float(cpmfactor); break;
    case 4: break;
    case 5: break;
  }
}

void menu_next()
{
  menu_idx++;
  if (menu_idx >= MENU_ITEMS) menu_idx=0;
  menu_level();
}

void menu_mode(uint8_t m)
{
  if (m==1 && menu_mdf!=2) menu_idx=0;
  menu_mdf=m;
  if (m==0) menu_poptime=0, lcd_msg(0);
  else if (m==1) menu_level();
  else if (m==2) menu_level();
}

void menu_cmd(byte c)
{
  if (menu_mdf==0) switch(c) {
    case BTN_CLICK: use_cpm = use_cpm? 0:1; menu_popup(use_cpm?msg_cpm:msg_uSv); break;
    case BTN_DCLICK: use_rcpm = use_rcpm? 0:1; menu_popup(use_rcpm?msg_rcpm:msg_icpm); break;
    case BTN_MCLICK: menu_mode(1); break;
    case BTN_LONG: powerdown(); break;
  } else if (menu_mdf==1) switch(c) {
    case BTN_CLICK: menu_next(); break;
    case BTN_DCLICK: menu_mode(2); break;
    case BTN_MCLICK: menu_mode(0); break;
    case BTN_LONG: powerdown(); break;
  } else if (menu_mdf==2) switch(c) {
    case BTN_CLICK: menu_action(1); break;
    case BTN_DCLICK: menu_action(2); break;
    case BTN_MCLICK: menu_mode(1); break;
    case BTN_LONG: powerdown(); break;
  } else menu_mode(0);
}

void setup()
{
  
#ifdef USE_POWERBUTTON
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, 1);
  
  for(int i=10; i--; delay(BTN_PERIOD)) btn.debounce();
  while(btn.debounce()==1); // wait release
#endif

#ifdef USE_SERIAL
  MYSERIAL.begin(57600);
#endif

#ifdef USE_CONVERTER
  analogReference(INTERNAL);
#endif

//  panel.rightToLeft();
//3  #1  PD3:OC2B
//5  #9  PD5:OC0B
//6  #10 PD6:OC0A
// basic:
//9  #13 PB1:OC1A
//10 #14 PB2:OC1B
//11 #15 PB3:OC2A


  TCCR1B = (TCCR1B & 0b11111000) | 0b001; // set PWM frequency @ 31250 Hz for Pins 9 and 10, MODE ?
  //TCCR2B = (TCCR2B & 0b11111000) | 0b001; // set PWM frequency @ 31250 Hz for Pins 11 and 3 (3 not used)

// PIN9 PB1 TIMER1A // mode 1000: phase correct, ICR1
//  analogWrite(9, 128);
  TCCR1B = (TCCR1B & 0b11000000) | _BV(WGM13) | 0b001; // mode 1000, CS1-001(clk)
  TCCR1A = _BV(COM1A1) | (0b00);  // mode 1000
  ICR1 = 1023 ; // 10   bit resolution
  OCR1A = 1; 
  pinMode(CONVERTER_PWM_PIN, OUTPUT);

#ifdef USE_EEPROM  
  lpar();
#endif

#ifdef USE_DISPLAY
  lcd_float(123.4);
#endif

#ifdef USE_COUNTER
  ir = 0;
  pinMode(LED_PIN, OUTPUT);
  pinMode(COUNTER_TRIG_PIN, INPUT); // pinD3, int1
  attachInterrupt(COUNTER_INTR, gm_pulse, FALLING);
#endif
}


#ifdef USE_SERIAL
void i_serial(char c)
{
  int x1;
  float x2; char x3[8];

  switch(c){
    case 'o':    x1 = MYSERIAL.parseInt();  MYSERIAL.print(":ocr");    MYSERIAL.println(x1);  OCR1A=x1; break;
    case 'i':    x1 = MYSERIAL.parseInt();  MYSERIAL.print(":icr");    MYSERIAL.println(x1);  ICR1 =x1; break;
#ifdef USE_CONVERTER
    case 'p':    x2 = MYSERIAL.parseFloat();  MYSERIAL.print(":kp");    MYSERIAL.println(x2);  kp=x2; break;
    case 't':    x2 = MYSERIAL.parseFloat();  MYSERIAL.print(":ki");    MYSERIAL.println(x2);  ki=x2; break;
    case 's':    x2 = MYSERIAL.parseFloat();  MYSERIAL.print(":sp");    MYSERIAL.println(x2);  setp=x2; break;
#endif
#ifdef USE_DISPLAY
    case 'd':    x2 = MYSERIAL.parseFloat();  MYSERIAL.print(":d");     MYSERIAL.println(x2);  lcd_float(x2); break;
    case 'D':    x1=0; while(MYSERIAL.available()) x3[x1++]=MYSERIAL.read(); x3[x1]='\0'; lcd_str(x3); break;
#endif
#ifdef USE_COUNTER
    case 'f':   x2 = MYSERIAL.parseFloat();  MYSERIAL.print(":f");     MYSERIAL.println(x2,6); cpmfactor=x2; break;
    case 'c':   x1 = MYSERIAL.parseInt();  MYSERIAL.print(":c");      MYSERIAL.println(x1);  use_rcpm=x1; break;    
#endif
#ifdef USE_EEPROM
    case 'Z':  clrpar(); break;
    case 'W':  wpar();  break;
#ifdef USE_SERIAL
 /// muahahha USE_SERIAL?? :^)
    case 'X':  par_show(); break;
#endif
#endif
    case '?':   
     MYSERIAL.print(":ocr");    MYSERIAL.println(OCR1A);
     MYSERIAL.print(":icr");    MYSERIAL.println(ICR1);
#ifdef USE_CONVERTER
     MYSERIAL.print(":kp");    MYSERIAL.println(kp);
     MYSERIAL.print(":ki");    MYSERIAL.println(ki);
     MYSERIAL.print(":sp");    MYSERIAL.println(setp);
     MYSERIAL.print(":u");     MYSERIAL.println(u*CONVERTER_MULT);
#endif
#ifdef USE_COUNTER
     MYSERIAL.print(":f");     MYSERIAL.println(cpmfactor,6);
     MYSERIAL.print(":c");      MYSERIAL.println(use_rcpm);
#endif

     break;
  }
}
#endif

void loop()
{
  uint32_t t;
  t = millis();

#ifdef USE_SERIAL
  if (MYSERIAL.available()) i_serial(MYSERIAL.read());
#endif

#ifdef USE_CONVERTER
  ustab();
#endif

#ifdef USE_COUNTER
  if (ir) ir=0, gm_intervals();
  if(t-tcpm >= 10000) tcpm=t, gm_counter();
#endif

#ifdef USE_DISPLAY
  if(t-slcd >= DISPLAY_NEW_MS) {
    slcd=t;
#ifdef USE_POWERBUTTON
    if (menu_poptime) {
      if (--menu_poptime==0) lcd_msg(0);
    }
#endif

#ifdef USE_CONVERTER
    ustab_show();
#endif
#ifdef USE_COUNTER
    gm_show();
#endif
  }

  if(t-tlcd >= DISPLAY_REFRESH_MS) tlcd=t,  lcd_refresh();
#endif /* USE_DISPLAY */

#ifdef USE_POWERBUTTON
  if (t-btc >= BTN_PERIOD) btc=t, menu_cmd(btn.poll());
#endif
}

void  counter_value(float n)
{
#ifdef USE_DISPLAY
  if(olcd)lcd_float(n);
#endif
}

void counter_int(uint16_t n)
{
#ifdef USE_DISPLAY
  if(olcd)lcd_int(n);
#endif
}

