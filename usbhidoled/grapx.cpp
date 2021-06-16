#include <Arduino.h>
#include "U8glib.h"

#define WIDTH 128
#define HEIGHT 32
#define PAGE_HEIGHT 8

/* init sequence adafruit 128x32 OLED (TESTED - WORKING 23.02.13), like adafruit3, but with page addressing mode */
static const uint8_t ssd1306_128x32_adafruit3_init_seq[] PROGMEM = {
  U8G_ESC_CS(0),        /* disable chip */
  U8G_ESC_ADR(0),       /* instruction mode */
  U8G_ESC_RST(1),    /* do reset low pulse with (1*16)+2 milliseconds */
  U8G_ESC_CS(1),        /* enable chip */

  0x0ae,        /* display off, sleep mode */
  0x0d5, 0x080,     /* clock divide ratio (0x00=1) and oscillator frequency (0x8) */
  0x0a8, 0x01f,     /* Feb 23, 2013: 128x32 OLED: 0x01f,  128x32 OLED 0x03f */

  0x0d3, 0x000,     /*  */

  0x040,        /* start line */
  
  0x08d, 0x014,     /* [2] charge pump setting (p62): 0x014 enable, 0x010 disable */ 

  0x020, 0x002,     /* com pin HW config, sequential com pin config (bit 4), disable left/right remap (bit 5), Feb 23, 2013: 128x32 OLED: 0x002,  128x32 OLED 0x012 */
  0x0a1,        /* segment remap a0/a1*/
  0x0c8,        /* c0: scan dir normal, c8: reverse */
  0x0da, 0x002,     /* com pin HW config, sequential com pin config (bit 4), disable left/right remap (bit 5) */
  0x081, 0x0cf,     /* [2] set contrast control */
  0x0d9, 0x0f1,     /* [2] pre-charge period 0x022/f1*/
  0x0db, 0x040,     /* vcomh deselect level */
  
  0x02e,        /* 2012-05-27: Deactivate scroll */ 
  0x0a4,        /* output ram to display */
  0x0a6,        /* none inverted normal display mode */
  0x0af,        /* display on */

  U8G_ESC_CS(0),        /* disable chip */
  U8G_ESC_END           /* end of sequence */
};

static const uint8_t ssd1306_128x32_data_start[] PROGMEM = {
  U8G_ESC_ADR(0),       /* instruction mode */
  U8G_ESC_CS(1),        /* enable chip */
  0x010,        /* set upper 4 bit of the col adr. to 0 */
  0x000,        /* set lower 4 bit of the col adr. to 4  */
  U8G_ESC_END           /* end of sequence */
};

static const uint8_t ssd13xx_sleep_on[] PROGMEM = {
  U8G_ESC_ADR(0),           /* instruction mode */
  U8G_ESC_CS(1),             /* enable chip */
  0x0ae,    /* display off */      
  U8G_ESC_CS(0),             /* disable chip, bugfix 12 nov 2014 */
  U8G_ESC_END                /* end of sequence */
};

static const uint8_t ssd13xx_sleep_off[] PROGMEM = {
  U8G_ESC_ADR(0),           /* instruction mode */
  U8G_ESC_CS(1),             /* enable chip */
  0x0af,    /* display on */      
  U8G_ESC_DLY(50),       /* delay 50 ms */
  U8G_ESC_CS(0),             /* disable chip, bugfix 12 nov 2014 */
  U8G_ESC_END                /* end of sequence */
};


uint8_t ssd1306_128x32_fn(u8g_t *u8g, u8g_dev_t *dev, uint8_t msg, void *arg)
{
  switch(msg)
  {
    case U8G_DEV_MSG_INIT:
      u8g_InitCom(u8g, dev, U8G_SPI_CLK_CYCLE_300NS);
      u8g_WriteEscSeqP(u8g, dev, ssd1306_128x32_adafruit3_init_seq);
      break;
    case U8G_DEV_MSG_STOP:
      break;
    case U8G_DEV_MSG_PAGE_NEXT:
      {
        u8g_pb_t *pb = (u8g_pb_t *)(dev->dev_mem);
        u8g_WriteEscSeqP(u8g, dev, ssd1306_128x32_data_start);    
        u8g_WriteByte(u8g, dev, 0x0b0 | pb->p.page);  /* select current page (SSD1306) */
        u8g_SetAddress(u8g, dev, 1);          /* data mode */
        if ( u8g_pb_WriteBuffer(pb, u8g, dev) == 0 )
          return 0;
        u8g_SetChipSelect(u8g, dev, 0);
      }
      break;
    case U8G_DEV_MSG_CONTRAST:
      u8g_SetChipSelect(u8g, dev, 1);
      u8g_SetAddress(u8g, dev, 0);          /* instruction mode */
      u8g_WriteByte(u8g, dev, 0x081);
      u8g_WriteByte(u8g, dev, (*(uint8_t *)arg) ); /* 11 Jul 2015: fixed contrast calculation */
      u8g_SetChipSelect(u8g, dev, 0);      
      return 1; 
    case U8G_DEV_MSG_SLEEP_ON:
      u8g_WriteEscSeqP(u8g, dev, ssd13xx_sleep_on);    
      return 1;
    case U8G_DEV_MSG_SLEEP_OFF:
      u8g_WriteEscSeqP(u8g, dev, ssd13xx_sleep_off);    
      return 1;
}
  
  return u8g_dev_pb8v1_base_fn(u8g, dev, msg, arg);
}

#define I2C_SLA         (0x3c*2)
//#define I2C_CMD_MODE  0x080
#define I2C_CMD_MODE    0x000
#define I2C_DATA_MODE   0x040

uint8_t com_arduino_ssd_start_sequence(u8g_t *u8g)
{
  /* are we requested to set the a0 state? */
  if(u8g->pin_list[U8G_PI_SET_A0] == 0) return 1;

  /* setup bus, might be a repeated start */
  if(u8g_i2c_start(I2C_SLA) == 0) return 0;
  if(u8g->pin_list[U8G_PI_A0_STATE] == 0) {
    if (u8g_i2c_send_byte(I2C_CMD_MODE) == 0)  return 0;
  } else {
    if (u8g_i2c_send_byte(I2C_DATA_MODE) == 0) return 0;
  }
  u8g->pin_list[U8G_PI_SET_A0] = 0;
  return 1;
}

uint8_t com_arduino_ssd_i2c_fn(u8g_t *u8g, uint8_t msg, uint8_t arg_val, void *arg_ptr)
{
  register uint8_t *ptr = arg_ptr;
  switch(msg) {
    case U8G_COM_MSG_INIT:
      u8g_i2c_init(u8g->pin_list[U8G_PI_I2C_OPTION]);
      break;
    case U8G_COM_MSG_STOP:
      break;
    case U8G_COM_MSG_RESET:
      break;
    case U8G_COM_MSG_CHIP_SELECT:
      u8g->pin_list[U8G_PI_A0_STATE] = 0;
      u8g->pin_list[U8G_PI_SET_A0] = 1;    /* force a0 to set again, also forces start condition */
      if(arg_val == 0) /* disable chip, send stop condition */
        u8g_i2c_stop();/* enable, do nothing: any byte writing will trigger the i2c start */
      break;
    case U8G_COM_MSG_WRITE_BYTE:
      if (com_arduino_ssd_start_sequence(u8g) == 0) return u8g_i2c_stop(), 0;
      if (u8g_i2c_send_byte(arg_val) == 0 ) return u8g_i2c_stop(), 0;
      break;
    case U8G_COM_MSG_WRITE_SEQ:
      if(com_arduino_ssd_start_sequence(u8g) == 0) return u8g_i2c_stop(), 0;
      while(arg_val > 0 ) {
        if(u8g_i2c_send_byte(*ptr++) == 0) return u8g_i2c_stop(), 0;
        arg_val--;
      }
      break;
    case U8G_COM_MSG_WRITE_SEQ_P:
      if(com_arduino_ssd_start_sequence(u8g) == 0) return u8g_i2c_stop(), 0;
      while(arg_val > 0) {
        if (u8g_i2c_send_byte(u8g_pgm_read(ptr)) == 0) return 0;
        ptr++;
        arg_val--;
      }
      break;
    case U8G_COM_MSG_ADDRESS:                     /* define cmd (arg_val = 0) or data mode (arg_val = 1) */
      u8g->pin_list[U8G_PI_A0_STATE] = arg_val;
      u8g->pin_list[U8G_PI_SET_A0] = 1;   /* force a0 to set again */
      break;
  }
  return 1;
}

uint8_t buf_ssd1306_128x32[WIDTH] U8G_NOCOMMON ; 
u8g_pb_t pb_ssd1306_128x32 = { {PAGE_HEIGHT, HEIGHT, 0, 0, 0},  WIDTH, buf_ssd1306_128x32}; 
u8g_dev_t dev_ssd1306_128x32_i2c = { ssd1306_128x32_fn, &pb_ssd1306_128x32, com_arduino_ssd_i2c_fn };

static u8g_t y8;
static u8g_uint_t tx, ty;          // current position for the Print base class procedures
static uint8_t is_begin;

/*
 * Test suite
 */

void u8g_prepare(void)
{
  u8g_SetFont(&y8, u8g_font_6x10);
  u8g_SetFontRefHeightExtendedText(&y8);
  u8g_SetDefaultForegroundColor(&y8);
  u8g_SetFontPosTop(&y8);
}

void u8g_box_frame(uint8_t a)
{
  u8g_DrawStr(&y8, 0, 0, "drawBox");
  u8g_DrawBox(&y8, 5,10,20,10);
  u8g_DrawBox(&y8, 10+a,15,30,7);
  u8g_DrawStr(&y8, 0, 30, "drawFrame");
  u8g_DrawFrame(&y8, 5,10+30,20,10);
  u8g_DrawFrame(&y8, 10+a,15+30,30,7);
}

void u8g_disc_circle(uint8_t a)
{
  u8g_DrawStr(&y8, 0, 0, "drawDisc");
  u8g_DrawDisc(&y8, 10,18,9, U8G_DRAW_ALL);
  u8g_DrawDisc(&y8, 24+a,16,7, U8G_DRAW_ALL);
  u8g_DrawStr(&y8, 0, 30, "drawCircle");
  u8g_DrawCircle(&y8, 10,18+30,9, U8G_DRAW_ALL);
  u8g_DrawCircle(&y8, 24+a,16+30,7, U8G_DRAW_ALL);
}

void u8g_r_frame(uint8_t a)
{
  u8g_DrawStr(&y8, 0, 0, "drawRFrame/Box");
  u8g_DrawRFrame(&y8, 5, 10,40,30, a+1);
  u8g_DrawRBox(&y8, 50, 10,25,40, a+1);
}

void u8g_string(uint8_t a)
{
  u8g_DrawStr(&y8,30+a,31, " 0");
  u8g_DrawStr90(&y8,30,31+a, " 90");
  u8g_DrawStr180(&y8,30-a,31, " 180");
  u8g_DrawStr270(&y8,30,31-a, " 270");
}

void u8g_line(uint8_t a)
{
  u8g_DrawStr(&y8, 0, 0, "drawLine");
  u8g_DrawLine(&y8, 7+a, 10, 40, 55);
  u8g_DrawLine(&y8, 7+a*2, 10, 60, 55);
  u8g_DrawLine(&y8, 7+a*3, 10, 80, 55);
  u8g_DrawLine(&y8, 7+a*4, 10, 100, 55);
}

void u8g_triangle(uint8_t a)
{
  uint16_t offset = a;
  u8g_DrawStr(&y8, 0, 0, "drawTriangle");
  u8g_DrawTriangle(&y8, 14,7, 45,30, 10,40);
  u8g_DrawTriangle(&y8, 14+offset,7-offset, 45+offset,30-offset, 57+offset,10-offset);
  u8g_DrawTriangle(&y8, 57+offset*2,10, 45+offset*2,30, 86+offset*2,53);
  u8g_DrawTriangle(&y8, 10+offset,40+offset, 45+offset,30+offset, 86+offset,53+offset);
}

void u8g_ascii_1()
{
  char s[2] = " ";
  uint8_t x, y;
  u8g_DrawStr(&y8, 0, 0, "ASCII page 1");
  for( y = 0; y < 6; y++ ) {
    for( x = 0; x < 16; x++ ) {
      s[0] = y*16 + x + 32;
      u8g_DrawStr(&y8, x*7, y*10+10, s);
    }
  }
}

void u8g_ascii_2()
{
  char s[2] = " ";
  uint8_t x, y;
  u8g_DrawStr(&y8, 0, 0, "ASCII page 2");
  for( y = 0; y < 6; y++ ) {
    for( x = 0; x < 16; x++ ) {
      s[0] = y*16 + x + 160;
      u8g_DrawStr(&y8, x*7, y*10+10, s);
    }
  }
}

void u8g_extra_page(uint8_t a)
{
    u8g_DrawStr(&y8, 0, 12, "setScale2x2");
    u8g_SetScale2x2(&y8);
    u8g_DrawStr(&y8, 0, 6+a, "setScale2x2");
    u8g_UndoScale(&y8);
}

uint8_t draw_state = 0;

void draw(void) {
  u8g_prepare();
  switch(draw_state >> 3) {
    case 0: u8g_box_frame(draw_state&7); break;
    case 1: u8g_disc_circle(draw_state&7); break;
    case 2: u8g_r_frame(draw_state&7); break;
    case 3: u8g_string(draw_state&7); break;
    case 4: u8g_line(draw_state&7); break;
    case 5: u8g_triangle(draw_state&7); break;
    case 6: u8g_ascii_1(); break;
    case 7: u8g_ascii_2(); break;
    case 8: u8g_extra_page(draw_state&7); break;
  }
}

void graph_setup()
{
  tx = 0; ty = 0; is_begin = 0;
  u8g_InitI2C(&y8,&dev_ssd1306_128x32_i2c,U8G_I2C_OPT_FAST);
}

void graph_loop()
{
    u8g_FirstPage(&y8);
    do draw(); while( u8g_NextPage(&y8) );
    if(++draw_state >= 9*8) draw_state=0;
}
