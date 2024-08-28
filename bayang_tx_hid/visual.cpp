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
#define DPG_TEST 0
#define DPG_MAIN 1
#define DPG_MENU 2
#define DPG_SHXY 3

#define HMENU_ITEMS 4
#define MENU_ITEMS 8

volatile uint16_t v_keys;
volatile uint16_t v_idx;

void cmd_bind();
void cmd_next_proto();
void cmd_showxy();

void cmd_m_menu();
void cmd_m_ok();
void cmd_m_up();
void cmd_m_down();
void cmd_m_exit();

static char *hmenu1[]={"Bind","Prot","vkey","Menu"};
static char *hmenu2[]={" ok "," up "," dn ","exit"};
static char *menu2[]={"ShowXY","TransmID","Bayg.opt","Syma.opt",NULL};
static void (*hmenu1_cmd[])()={cmd_bind, cmd_next_proto, NULL, cmd_m_menu};
static void (*hmenu2_cmd[])()={cmd_m_ok, cmd_m_up, cmd_m_down, cmd_m_exit};
static void (*menu2_cmd[])()={cmd_showxy, NULL};

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
      saved_proto(EEPROM_PROTO_OFFSET,get_protocol());
    } else nx=next_protocol();
  } 
}

void cmd_showxy()
{ 
  d_page=DPG_SHXY;
}

void cmd_m_menu() { d_page=DPG_MENU; v_idx=0; }
void cmd_m_ok() { }
void cmd_m_up() { v_idx--; v_idx &= MENU_ITEMS-1; }
void cmd_m_down() { v_idx++; v_idx &= MENU_ITEMS-1; }
void cmd_m_exit() { d_page=1; }

uint8_t item_key(int i) { return (i<2)?(i==0?KEY_1:KEY_2):(i==2?KEY_3:KEY_4); }

void menu_key_event(uint16_t k, uint16_t ch)
{
  void (**cmdlst)();

  v_update();
  switch (d_page) {
  default: // test screen
    cmdlst=NULL;
    break;
  case DPG_MAIN: // main screen
    cmdlst=hmenu1_cmd;
    break;
  case DPG_MENU: // main menu
    cmdlst=hmenu2_cmd;
    break;
  }

  for(int i = 0; i < HMENU_ITEMS; i++) {
    uint8_t m=item_key(i);
    if (ch&k&m && cmdlst!=NULL && cmdlst[i]!=NULL)
      (cmdlst[i])();
  }
}

/*
 * display part
 */

#define MENU1_W (SCREEN_W/4)
#define MENU1_H (12)
#define MENU2_W (SCREEN_W/2)
void draw_hmenu(char **lst, uint8_t marks)
{
  for(int i = 0; i < HMENU_ITEMS; i++) {
    int x=MENU1_W*i - ((i==3)?1:0); // last item offset
    uint8_t m=item_key(i);
    u8g.drawStr(x, SCREEN_H-1, lst[i]);
    if (marks&m) u8g.drawFrame(x,SCREEN_H-MENU1_H,MENU1_W-1,MENU1_H-1);
  }
}

void draw_menu(char **lst, uint8_t n)
{
  for(int i = 0; i < MENU_ITEMS; i++) {
    if(lst[i]==NULL) break;
    int x=(i>=4)?MENU2_W-1:0;
    int y=(i%4)*MENU1_H;
    u8g.drawStr(x, y+MENU1_H-1, lst[i]);
    if (i==n) u8g.drawFrame(x,y,MENU2_W-1,MENU1_H-1);
  }
}

void draw(void)
{
  if (d_updid) u8g.drawPixel(SCREEN_W-1,0);
  switch (d_page) {
    case DPG_TEST: // test screen
      u8g.setFont(u8g_font_unifont); //(u8g_font_osb21);
      u8g.drawStr( 0, 22, "Hello World!");
      break;
    case DPG_MAIN: // main screen
      u8g.setFont(u8g_font_unifont); //(u8g_font_osb21);
      u8g.drawStr( 76, 12, (d_flags&dFL_VUSB)?((d_flags&dFL_USBSER)? "SERIAL":"USB"):"no usb");
      u8g.drawStr( 76, 24, (txstate)? (txstate==-2?"init":"bound"):"bind ");
      u8g.drawStr( 76, 36, get_protocol());
      draw_hmenu(hmenu1, v_keys&KEY_SYS);
      break;
    case DPG_MENU: // menu screen
      u8g.setFont(u8g_font_unifont); //(u8g_font_osb21);
      draw_menu(menu2,v_idx);
      draw_hmenu(hmenu2, v_keys&KEY_SYS);
      break;
    case DPG_SHXY: // XY screen
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
