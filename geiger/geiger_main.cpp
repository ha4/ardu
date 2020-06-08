
#include "geiger.h"

void setup()
{
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

if(t-slcd >= DISPLAY_NEW_MS) slcd=t, ustab_show(), gm_show();

#ifdef USE_DISPLAY
  if(t-tlcd >= DISPLAY_REFRESH_MS) tlcd=t,  lcd_refresh();
#endif

}
