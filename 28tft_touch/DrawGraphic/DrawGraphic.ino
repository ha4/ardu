#include <stdint.h>
#include <LCD.h>
#include <SPI.h>

/*
  LCD tft waveshare 2.8" toucscreen
  SPI (mosi,miso,sck) = PB3, PB4, PB5 = D 11,12,13
  LCD_CS, LCD_D/C, LCD_BLight = PB2(~ss), PD7, PB1 = D 10,7,9
  CD_CS   = PD5 = 5
  TP_CS, TP_IRQ = PD4, PD3(int1) = 4,3
 */
void setup()
{
    SPI.setDataMode(SPI_MODE3);
    SPI.setBitOrder(MSBFIRST);
    SPI.setClockDivider(SPI_CLOCK_DIV4);
    SPI.begin();
    
    Tft.lcd_init();                                      // init TFT library
    
    Tft.lcd_draw_rect(30, 40, 150, 100, RED);
    Tft.lcd_draw_circle(120, 160, 50, BLUE);
    Tft.lcd_draw_line(30, 40, 180, 140, RED);
    
    Tft.lcd_draw_line(30, 220, 210, 240, RED);
    Tft.lcd_draw_line(30, 220, 120, 280, RED);
    Tft.lcd_draw_line(120, 280, 210, 240, RED);
}

void loop()
{
  
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

