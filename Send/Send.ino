#include <easyTIMER.h>
#include <LCDnum.h>

// initialize the library with the numbers of the interface pins
enum { lcdSTB=9, lcdBASE=8 };

LCDnum panel(lcdSTB, lcdBASE);
easyTIMER t1(CLK_OSC_DIV8, CLK_M0_NORMAL, CLK_IRQ_OVERFLOW);


ISR(TIMER1_OVF_vect) // 16MHz/65536/8 = 30.5Hz
{
  panel.refresh(0);
}

word a;

void setup()
{
    a = 1;
    t1.clk_start();
    panel.noAutoscroll();
}

void loop()
{
    panel.println(a,HEX);   delay(200); a = a+1;
}

