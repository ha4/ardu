#include <Wire.h>
#include "U8glib.h"

#define WIDTH 128
#define HEIGHT 32
#define PAGE_HEIGHT 8

#define I2C_SLA         (0x3c*2)
//#define I2C_CMD_MODE  0x080
#define I2C_CMD_MODE    0x000
#define I2C_DATA_MODE   0x040

uint8_t ssd1306_128x32_fn(u8g_t *u8g, u8g_dev_t *dev, uint8_t msg, void *arg);
uint8_t com_arduino_ssd_i2c_fn(u8g_t *u8g, uint8_t msg, uint8_t arg_val, void *arg_ptr);

uint8_t buf_ssd1306_128x32[WIDTH] U8G_NOCOMMON ; 
u8g_pb_t pb_ssd1306_128x32 = { {PAGE_HEIGHT, HEIGHT, 0, 0, 0},  WIDTH, buf_ssd1306_128x32}; 
u8g_dev_t dev_ssd1306_128x32_i2c = { ssd1306_128x32_fn, &pb_ssd1306_128x32, com_arduino_ssd_i2c_fn };

static u8g_t y8;

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
  0x0a6,        /* 0xa6 none inverted normal display mode, 0xa7 inverted */
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

uint8_t _WriteEscSeqP(u8g_t *u8g, u8g_dev_t *dev, const uint8_t *esc_seq)
{
  uint8_t is_escape = 0;
  uint8_t value;
  for(;;) {
    value = u8g_pgm_read(esc_seq++);
    if(is_escape == 0) {
      if(value != 255) { if(dev->com_fn(u8g, U8G_COM_MSG_WRITE_BYTE, value, NULL) == 0) return 0; }
      else is_escape = 1;
      continue;
    }

    if(value == 255) { if(dev->com_fn(u8g, U8G_COM_MSG_WRITE_BYTE, value, NULL) == 0) return 0; }
    else if(value == 254)  { break; }
    else if(value >= 0x0f0){ /* not yet used, do nothing */ }
    else if(value >= 0xe0) { dev->com_fn(u8g, U8G_COM_MSG_ADDRESS, value & 0x0f, NULL); }
    else if(value >= 0xd0) { dev->com_fn(u8g, U8G_COM_MSG_CHIP_SELECT, value & 0x0f, NULL); }
    else if(value >= 0xc0) {
      dev->com_fn(u8g, U8G_COM_MSG_RESET, 0, NULL);
      value=2+((value & 0x0f) << 4);
      delay(value);
      dev->com_fn(u8g, U8G_COM_MSG_RESET, 1, NULL);
      delay(value);
    } else if(value >= 0xbe){ /* not yet implemented u8g_SetVCC(u8g, dev, value & 0x01); */ }
    else if ( value <= 127 ){ delay(value); }
    is_escape = 0;
  }
  return 1;
}

void _pb8v1_SetPixel(u8g_pb_t *b, const u8g_dev_arg_pixel_t * const arg_pixel)
{
  if (arg_pixel->y < b->p.page_y0) return;
  if (arg_pixel->y > b->p.page_y1) return;
  if (arg_pixel->x >= b->width)  return;
  register uint8_t mask=1<<((arg_pixel->y - b->p.page_y0)&7);
  uint8_t *ptr = b->buf + arg_pixel->x;
  if(arg_pixel->color) *ptr |= mask;
  else  *ptr &= mask^0xff;
}

void _pb8v1_Set8PixelOpt2(u8g_pb_t *b, u8g_dev_arg_pixel_t *arg_pixel)
{
  register uint8_t pixel = arg_pixel->pixel;
  u8g_uint_t dx = 0;
  u8g_uint_t dy = 0;

  switch(arg_pixel->dir)
    { case 0: dx++; break; case 1: dy++; break; case 2: dx--; break; case 3: dy--; break; }

  do {
    if (pixel & 128) _pb8v1_SetPixel(b, arg_pixel);
    arg_pixel->x += dx;
    arg_pixel->y += dy;
    pixel <<= 1;
  } while(pixel != 0);
}

void _pb_Clear(u8g_pb_t *b)
{
  uint8_t *ptr = (uint8_t *)b->buf;
  uint8_t *end_ptr = ptr;
  end_ptr += b->width;
  do *ptr++ = 0; while( ptr != end_ptr );
}

void _pb_GetPageBox(u8g_pb_t *pb, u8g_box_t *box)
{
  box->x0 = 0; box->y0 = pb->p.page_y0; 
  box->x1 = pb->width; box->x1--; box->y1 = pb->p.page_y1; 
}

uint8_t _pb_Is8PixelVisible(u8g_pb_t *b, u8g_dev_arg_pixel_t *arg_pixel)
{
  u8g_uint_t v0, v1;
  v0 = arg_pixel->y;
  v1 = v0;
  switch(arg_pixel->dir)
  {
    case 0: break;
    case 1: v1 += 8; /* this is independent from the page height */  break;
    case 2: break;
    case 3: v0 -= 8; break;
  }
  uint8_t c1, c2, c3, tmp;
  c1 = v0 <= b->p.page_y1;
  c2 = v1 >= b->p.page_y0;
  c3 = v0 > v1;
  tmp = c1;
  c1 &= c2;
  c2 &= c3;
  c3 &= tmp;
  c1 |= c2;
  c1 |= c3;
  return c1 & 1;
}

void _page_First(u8g_page_t *p)
{
  p->page_y0=0;
  p->page_y1=p->page_height;
  p->page_y1--;
  p->page = 0;
}

uint8_t _page_Next(u8g_page_t * p)
{
  register u8g_uint_t y1;
  p->page_y0 += p->page_height;
  if(p->page_y0 >= p->total_height) return 0;
  p->page++;
  y1 = p->page_y1;
  y1 += p->page_height;
  if(y1 >= p->total_height) {
    y1 = p->total_height;
    y1--;
  }
  p->page_y1 = y1;
  return 1;
}

uint8_t ssd1306_128x32_fn(u8g_t *u8g, u8g_dev_t *dev, uint8_t msg, void *arg)
{
  u8g_pb_t *pb = (u8g_pb_t *)(dev->dev_mem);
  switch(msg)
  {
    case U8G_DEV_MSG_INIT:
      dev->com_fn(u8g, U8G_COM_MSG_INIT, U8G_SPI_CLK_CYCLE_300NS, NULL); // init com
      _WriteEscSeqP(u8g, dev, ssd1306_128x32_adafruit3_init_seq);
      break;
    case U8G_DEV_MSG_STOP:
      break;
    case U8G_DEV_MSG_PAGE_NEXT:
      _WriteEscSeqP(u8g, dev, ssd1306_128x32_data_start);
      dev->com_fn(u8g, U8G_COM_MSG_WRITE_BYTE, 0x0b0 | pb->p.page, NULL); /* select current page (SSD1306) */
      dev->com_fn(u8g, U8G_COM_MSG_ADDRESS, 1, NULL);          /* data mode */
      if(dev->com_fn(u8g, U8G_COM_MSG_WRITE_SEQ, pb->width, pb->buf) == 0)
        return 0;
      dev->com_fn(u8g, U8G_COM_MSG_CHIP_SELECT, 0, NULL);
      break;
    case U8G_DEV_MSG_CONTRAST:
      dev->com_fn(u8g, U8G_COM_MSG_CHIP_SELECT, 1, NULL);
      dev->com_fn(u8g, U8G_COM_MSG_ADDRESS, 0, NULL);     /* instruction mode */
      dev->com_fn(u8g, U8G_COM_MSG_WRITE_BYTE, 0x081, NULL);
      dev->com_fn(u8g, U8G_COM_MSG_WRITE_BYTE, (*(uint8_t *)arg), NULL); /* 11 Jul 2015: fixed contrast calculation */
      dev->com_fn(u8g, U8G_COM_MSG_CHIP_SELECT, 0, NULL);
      return 1; 
    case U8G_DEV_MSG_SLEEP_ON:
      _WriteEscSeqP(u8g, dev, ssd13xx_sleep_on);    
      return 1;
    case U8G_DEV_MSG_SLEEP_OFF:
      _WriteEscSeqP(u8g, dev, ssd13xx_sleep_off);    
      return 1;
  }
  
  switch(msg){
    case U8G_DEV_MSG_SET_8PIXEL:
        if (_pb_Is8PixelVisible(pb, (u8g_dev_arg_pixel_t *)arg))
        _pb8v1_Set8PixelOpt2(pb, (u8g_dev_arg_pixel_t *)arg);
      break;
    case U8G_DEV_MSG_SET_PIXEL: _pb8v1_SetPixel(pb, (u8g_dev_arg_pixel_t *)arg);  break;
    case U8G_DEV_MSG_PAGE_FIRST: _pb_Clear(pb); _page_First(&(pb->p)); break;
    case U8G_DEV_MSG_PAGE_NEXT: if (_page_Next(&(pb->p)) == 0) return 0; _pb_Clear(pb); break;
    case U8G_DEV_MSG_GET_PAGE_BOX: _pb_GetPageBox(pb, (u8g_box_t *)arg);  break;
    case U8G_DEV_MSG_GET_WIDTH:  *((u8g_uint_t *)arg) = pb->width;   break;
    case U8G_DEV_MSG_GET_HEIGHT: *((u8g_uint_t *)arg) = pb->p.total_height;  break;
    case U8G_DEV_MSG_GET_MODE: return U8G_MODE_BW;
  }
  return 1;
}


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
    case U8G_COM_MSG_ADDRESS:    /* define cmd (arg_val = 0) or data mode (arg_val = 1) */
      u8g->pin_list[U8G_PI_A0_STATE] = arg_val;
      u8g->pin_list[U8G_PI_SET_A0] = 1;   /* force a0 to set again */
      break;
  }
  return 1;
}

/* zaames */

static const uint8_t font6x8_dig[] PROGMEM = {
  0, 0, 0, 0, 0, 0, 0, 0,
  112, 136, 168, 168, 136, 112, 0, 0, /*0*/
  32, 96, 32, 32, 32, 112, 0, 0,
  112, 136, 8, 112, 128, 248, 0, 0,
  248, 8, 48, 8, 136, 112, 0, 0,
  32, 96, 160, 248, 32, 32, 0, 0,
  248, 128, 112, 8, 136, 112, 0, 0,
  48, 64, 112, 136, 136, 112, 0, 0,
  248, 136, 16, 32, 32, 32, 0, 0,
  112, 136, 112, 136, 136, 112, 0, 0,
  112, 136, 136, 120, 8, 112, 0, 0, /*9*/
  0, 32, 32, 248, 32, 32, 0, 0, /*+*/
  0, 0, 0, 248, 0, 0, 0, 0,     /*-*/
  0, 0, 0, 0, 0, 64, 0, 0,      /*.*/ 
};

u8g_pgm_uint8_t *_glyphdata(uint8_t c)
{
  uint16_t d=9;
  if(c >='0' && c <='9') d=(c-'0'+1)*8;
  else if(c =='+') d=(11)*8;
  else if(c =='-') d=(12)*8;
  else if(c =='.') d=(13)*8;
  return font6x8_dig+d;
}

void _Draw8Pixel(u8g_t *u8g, u8g_dev_t *dev, u8g_uint_t x, u8g_uint_t y, uint8_t dir, uint8_t pixel)
{
  u8g_dev_arg_pixel_t *arg = &(u8g->arg_pixel);
  arg->x = x;
  arg->y = y;
  arg->dir = dir;
  arg->pixel = pixel;
  u8g_call_dev_fn(u8g, dev, U8G_DEV_MSG_SET_8PIXEL, arg);
}

static uint8_t __inline__ __attribute__((always_inline)) 
   _is_intersect(u8g_uint_t a0, u8g_uint_t a1, u8g_uint_t v0, u8g_uint_t v1) 
{
  if(v0 <= a1) {
    if(v1 >= a0) return 1;
    else { if(v0 > v1) return 1; else return 0; }
  } else {
    if(v1 >= a0) { if(v0 > v1) return 1;  else return 0; }
    else return 0;
  }
}

uint8_t _IsBBXIntersection(u8g_t *u8g, u8g_uint_t x, u8g_uint_t y, u8g_uint_t w, u8g_uint_t h)
{
  register u8g_uint_t e=y+h;
  if(_is_intersect(u8g->current_page.y0, u8g->current_page.y1, y, --e) == 0)
    return 0; 
  e = x+w;
  return _is_intersect(u8g->current_page.x0, u8g->current_page.x1, x, --e);
}

int8_t _drawGlyph6(u8g_t *u8g, u8g_uint_t x, u8g_uint_t y, uint8_t encoding)
{
  const u8g_pgm_uint8_t *data;
  uint8_t w, h;
  
  data = _glyphdata(encoding);
  w = 6;
  h = 8;

  if(_IsBBXIntersection(u8g, x, y, w, h) == 0) return w;
  for(int j=0; j < h; j++, y++)
      _Draw8Pixel(u8g,u8g->dev, x, y, 0, u8g_pgm_read(data++));
  return w;
}

void my_DrawStr(u8g_t *u8g, u8g_uint_t x, u8g_uint_t y, const char *s)
{
  int8_t d;
  while(*s != '\0') {
    d = _drawGlyph6(u8g, x, y, *s++);
    x += d;
  }
}

void _firstPage(u8g_t *u8g)
{
  u8g_call_dev_fn(u8g, u8g->dev, U8G_DEV_MSG_PAGE_FIRST, NULL);
  u8g_call_dev_fn(u8g, u8g->dev, U8G_DEV_MSG_GET_PAGE_BOX, &(u8g->current_page));  
}

uint8_t _nextPage(u8g_t *u8g)
{
  uint8_t r;
  if(u8g->cursor_fn)
    u8g->cursor_fn(u8g);
  r=u8g_call_dev_fn(u8g, u8g->dev, U8G_DEV_MSG_PAGE_NEXT, NULL);
  if(r)
    u8g_call_dev_fn(u8g, u8g->dev, U8G_DEV_MSG_GET_PAGE_BOX, &(u8g->current_page));
  return r;
}

/*callbacks*/
u8g_uint_t _vref_font(u8g_t *u8g) { return 0; }
void _dummystate_cb(uint8_t msg) { }

void _init_data(u8g_t *u8g)
{
  u8g->font = NULL;
  u8g->cursor_font = NULL;
  u8g->cursor_bg_color = 0;
  u8g->cursor_fg_color = 1;
  u8g->cursor_encoding = 34;
  u8g->cursor_fn = (u8g_draw_cursor_fn)0;

  for(uint8_t i = 0; i < U8G_PIN_LIST_LEN; i++ )
    u8g->pin_list[i] = U8G_PIN_NONE;
  
  u8g->arg_pixel.color = 1; // set color index
  u8g->font_calc_vref = _vref_font; // setfontpos_baseline
  
  u8g->font_height_mode = U8G_FONT_HEIGHT_MODE_XTEXT;
  u8g->font_ref_ascent = 0;
  u8g->font_ref_descent = 0;
  u8g->font_line_spacing_factor = 64;           /* 64 = 1.0, 77 = 1.2 line spacing factor */
  u8g->line_spacing = 0;
  
  u8g->state_cb = _dummystate_cb;
}

void _initGC(u8g_t *u8g)
{
  u8g_uint_t r;
 
  _init_data(u8g);
  u8g->dev = &dev_ssd1306_128x32_i2c;
  u8g->pin_list[U8G_PI_I2C_OPTION] = U8G_I2C_OPT_FAST;

  // begin
  if(u8g_call_dev_fn(u8g, u8g->dev, U8G_DEV_MSG_INIT, NULL) == 0)
    return ;
  /* fetch width and height from the low level */
  u8g_call_dev_fn(u8g, u8g->dev, U8G_DEV_MSG_GET_WIDTH, &r);
  u8g->width = r;
  u8g_call_dev_fn(u8g, u8g->dev, U8G_DEV_MSG_GET_HEIGHT, &r);
  u8g->height = r;
  u8g->mode = u8g_call_dev_fn(u8g, u8g->dev, U8G_DEV_MSG_GET_MODE, NULL);
  u8g_call_dev_fn(u8g, u8g->dev, U8G_DEV_MSG_GET_PAGE_BOX, &(u8g->current_page));
}

void setup()
{
  _initGC(&y8);
}

void loop()
{
  static int cnt=0;
  static uint32_t t1=0;
  uint32_t t=micros();
  char v[20];
  t1=t-t1;
  if(t1>0)t1=10000000L/t1;
  itoa(t1/10,v,10);
  t1=t;

  _firstPage(&y8);
  do {
    my_DrawStr(&y8, 0, 0, v);
    itoa(cnt,v,10);       my_DrawStr(&y8, 64, 0, v);
    itoa(cnt,v,2);        my_DrawStr(&y8, 0, 17, v);
    itoa(32767-cnt,v,10); my_DrawStr(&y8, 32, 24, v);
  } while( _nextPage(&y8) );
  cnt++;
}
