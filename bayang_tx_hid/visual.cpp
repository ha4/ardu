#include <Arduino.h>
#include "inp_x.h"
#include "tx_util.h"
#include "visual.h"
#include "U8glib.h"

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_DEV_0 | U8G_I2C_OPT_FAST); // Dev 0, Fast I2C / TWI

volatile uint8_t d_updating;
volatile uint8_t d_page;
volatile uint16_t d_flags;
volatile uint8_t d_updid;

/*
 *  Menu interaction part
 */
volatile uint16_t v_keys;

void cmd_bind();
void cmd_next_proto();

void cmd_m_menu();
void cmd_m_ok();
void cmd_m_up();
void cmd_m_down();
void cmd_m_exit();

static char *menu1[]={"Bind","Prot","---","Menu"};
static char *menu2[]={" ok "," up "," dn ","exit"};
static void (*menu1_cmd[])()={cmd_bind, cmd_next_proto, NULL, cmd_m_menu};
static void (*menu2_cmd[])()={cmd_m_ok, cmd_m_up, cmd_m_down, cmd_m_exit};

void cmd_bind() { txstate=STATE_DOBIND; }

void cmd_next_proto()
{
  char *prt=get_protocol();
  char *nx;
  nx=next_protocol();
  while(nx != NULL) {
    if (strcmp(prt,nx)==0) {
      nx=next_protocol();
      set_protocol(nx);
    } else nx=next_protocol();
  } 
}


void cmd_m_menu() { d_page=2; }
void cmd_m_ok() { }
void cmd_m_up() { }
void cmd_m_down() { }
void cmd_m_exit() { d_page=1; }

uint8_t item_key(int i) { return (i<2)?(i==0?KEY_1:KEY_2):(i==2?KEY_3:KEY_4); }

void menu_command(uint16_t k, uint16_t ch)
{
  void (**cmdlst)();

  v_update();
  switch (d_page) {
  default: // test screen
    cmdlst=NULL;
    break;
  case 1: // main screen
    cmdlst=menu1_cmd;
    break;
  case 2: // main menu
    cmdlst=menu2_cmd;
    break;
  }

  for(int i = 0; i < 4; i++) {
    uint8_t m=item_key(i);
    if (ch&k&m && cmdlst!=NULL && cmdlst[i]!=NULL)
      (cmdlst[i])();
  }
}

/*
 * display part
 */
void draw_hmenu(char **lst, uint8_t marks)
{
  for(int i = 0; i < 4; i++) {
    int x=(i==3)?95:i*32;
    uint8_t m=item_key(i);
    u8g.drawStr(x, 63, lst[i]);
    if (marks&m) u8g.drawFrame(x,52,31,11);
  }
}

void draw(void)
{
  if (d_updid) u8g.drawPixel(127,0);
  switch (d_page) {
    case 0: // test screen
      u8g.setFont(u8g_font_unifont); //(u8g_font_osb21);
      u8g.drawStr( 0, 22, "Hello World!");
      break;
    case 1: // main screen
      u8g.setFont(u8g_font_unifont); //(u8g_font_osb21);
      u8g.drawStr( 76, 12, (d_flags&dFL_VUSB)?((d_flags&dFL_USBSER)? "SERIAL":"USB"):"no usb");
      u8g.drawStr( 76, 24, (txstate)? (txstate==-2?"init":"bound"):"bind ");
      u8g.drawStr( 76, 36, get_protocol());
      draw_hmenu(menu1, v_keys&KEY_SYS);
      break;
    case 2: // menu screen
      u8g.setFont(u8g_font_unifont); //(u8g_font_osb21);
      u8g.drawStr( 0, 12, "page2");
      draw_hmenu(menu2, v_keys&KEY_SYS);
      break;
    case 3: // XY screen
      u8g.setFont(u8g_font_unifont); //(u8g_font_osb21);
      u8g.drawStr( 0, 12, "page3");
      break;
  }
}

void v_init()
{
  u8g.setColorIndex(1);
  d_page = 1;
  v_update();
}

void v_loop()
{
  if (!d_updating) return;

  d_updating = 0;
  u8g.firstPage();
  do draw(); while (u8g.nextPage());
}

void v_update()
{
  d_updating = 1;
  d_updid = !d_updid;
}
