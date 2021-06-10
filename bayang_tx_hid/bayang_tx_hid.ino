#include "usb_gpad.h"
#include "tx_util.h"
#include "Bayang_nrf24l01.h"
#include "symax_nrf24l01.h"
#include "xtimer.h"
#include "inp_x.h"

// nrf24l0 pin YLW,CS=PB0(ss,d17) ORA,CE=PD4(d4) GRN,SCK=PB1(d15) BRN,MOSI=PB2(d16) BLK,MISO=PB3(d14) RED,VDD=+3.3 BLU,GND
// display sh1106   GRN,SCL=PD0(d3,scl) BRN,SDA=PD1(d2,sda) BLU,GND YLW,VDD=+3.3
// key-rows 74hc164 pin 1&2 = DATA(d8), 8 - CLOCK(d6), pin 9 - CLEAR (+Vdd)
// key-columns pullup=10k COL(A)=(d5), COL(B)=(d7)

#include "U8glib.h"

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_DEV_0 | U8G_I2C_OPT_FAST); // Dev 0, Fast I2C / TWI

#define EEPROM_ID_OFFSET    10    // Module ID (4 bytes)

#define dFL_VUSB 0x0001
#define dFL_USBSER 0x0002
#define dFL_PROTO_SYMAX 0x0004

void none_data();
void bayang_data();
void symax_data();

struct proto_t {
  void (*init)();
  void (*bind)();
  uint16_t (*callback)();
  int8_t (*state)();
  void (*data)();
  char *name;
};

struct SymaXData txS;
struct BayangData txB;

struct proto_t tx_lst[] = {
  {&noneTX_init, &noneTX_bind, &noneTX_callback, &noneTX_state, &none_data, "NoTxm" },
  {&BAYANG_TX_init, &BAYANG_TX_bind, &BAYANG_TX_callback, &BAYANG_TX_state, &bayang_data, "Bayng" },
  {&symax_init, &symax_bind, &symax_callback, &symax_state, &symax_data, "SymaX" },  
};

xtimer txtimer; // Transmitter
xtimer potimer; // Pot read adc
xtimer prtimer; // serial print

volatile uint8_t d_updating, d_updid, d_page;
volatile uint16_t d_flags;

uint16_t keys;

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

#define STATE_DATA 0
#define STATE_BIND 1
#define STATE_ERR  -1
#define STATE_DOINIT -2
#define STATE_DOBIND -3

uint32_t MProtocol_id_master;
int8_t txstate;
struct proto_t *txproto;

#define TEST_LIM 511
#define TEST_SCA 128
#define TEST_SCB 32
#define TEST_CIRC(s,c,fs,fc,sc) fs=s/sc; fc=c/sc; s=limadd(s,fc); c=limadd(c,-fs)

int  limadd(int a, int b) { a+=b;  return max(-TEST_LIM,min(a,TEST_LIM)); }

int read_test(uint8_t chan)
{
    static int sc[] = {0, TEST_LIM, TEST_LIM, 0};
    int ts,tc;
    if (chan==0) {
      TEST_CIRC(sc[0],sc[1],ts,tc,TEST_SCA);
      TEST_CIRC(sc[2],sc[3],ts,tc,TEST_SCB);
    }
    return sc[chan];
}

/* conversion routines */

bool adc_scanall()
{
  bool c = 0;

  uint8_t b = 0;
  int16_t ry = adc_read(POT_ELEVATOR);
  int16_t rx = 1023-adc_read(POT_AILERON);
  int16_t ly = adc_read(POT_THROTTLE);
  int16_t lx = adc_read(POT_RUDDER);
//  lx-=512;  ly-=512;  rx-=512;  ry-=512; // +/-511 scale
//  lx*=64; ly*=64; rx*=64; ry*=64; // +/-32767 scale
//  lx*=4; ly*=4; rx*=4; ry*=4; // +/-2047 scale
//  lx+=1000; ly+=1000; rx+=1000; ry+=1000; // 1000..2000 pulse scale
//  lx/=4; ly/=4; rx/=4; ry/=4; // +/-127 scale
//  lx*=2; ly*=2; rx*=2; ry*=2; // 0..2048 scale

  if (b != rpt_data.buttons || 
      ry != rpt_data.rightY || rx != rpt_data.rightX ||
      ly != rpt_data.leftY || lx != rpt_data.leftX) {
    rpt_data.leftX = lx;
    rpt_data.leftY = ly;
    rpt_data.rightX = rx;
    rpt_data.rightY = ry;
    rpt_data.buttons = b;
    c = 1;
  }

  return c;
}

bool rpt_test()
{
    rpt_data.buttons = 0;
    rpt_data.leftY = read_test(0);
    rpt_data.leftX = read_test(1);
    rpt_data.rightY = read_test(2);
    rpt_data.rightX = read_test(3);

  return true;
}

void none_data() {}

void bayang_data()
{
  
}

void symax_data()
{
  uint8_t b = 0;
  uint16_t elevt = adc_read(POT_ELEVATOR);
  uint16_t ailrn = adc_read(POT_AILERON);
  uint16_t throt = adc_read(POT_THROTTLE);
  uint16_t rudd  = adc_read(POT_RUDDER);

  txS.aileron=(ailrn-512)/4;
  txS.elevator=(512-elevt)/4;
  txS.throttle=(1024-throt)/4-48;
  txS.rudder=(512-rudd)/4;
  txS.flags5=FLAG5_HIRATE;
}

void set_protocol(char *name)
{
  int i=sizeof(tx_lst)/sizeof(struct proto_t);
  txstate=STATE_DOINIT;
  if (name==NULL) { txproto=&tx_lst[0]; return; }
  size_t nl=strlen(name);
  if (nl==0) { txproto=&tx_lst[0]; return; }
  while(i) {
    i--;
    txproto=&tx_lst[i];
    if(strncmp(name,txproto->name,nl)==0) return;
  }
}

char *next_protocol()
{
  static uint8_t i=0;
  if(i < sizeof(tx_lst)/sizeof(struct proto_t)) return tx_lst[i++].name;
  i=0;
  return NULL;
}

char *get_protocol()
{
  return txproto->name;
}

void d_update()
{
  d_updating = 1;
  d_updid = !d_updid;
}

void draw_hmenu(char **lst, uint8_t marks)
{
  for(int i = 0; i < 4; i++) {
    int x=(i==3)?95:i*32;
    uint8_t m=(i<2)?(i==0?KEY_1:KEY_2):(i==2?KEY_3:KEY_4);
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
      draw_hmenu(menu1, keys&KEY_SYS);
      break;
    case 2: // menu screen
      u8g.setFont(u8g_font_unifont); //(u8g_font_osb21);
      u8g.drawStr( 0, 12, "page2");
      draw_hmenu(menu2, keys&KEY_SYS);
      break;
    case 3: // XY screen
      u8g.setFont(u8g_font_unifont); //(u8g_font_osb21);
      u8g.drawStr( 0, 12, "page3");
      break;
  }
}

void comm_usb()
{
   if(USBSTA&(1<<VBUS)) {
    if(d_flags&dFL_VUSB) return;
    d_flags|=dFL_VUSB;
    d_update();
   } else {
    if(!(d_flags&dFL_VUSB)) return;
    d_flags&=~dFL_VUSB;
    d_update();
   }
}

bool comm_serial()
{
  static bool sertostart = 1;
    if (Serial.dtr()) { // is port open
      if (sertostart) {
        sertostart = 0;
        Serial.begin(115200);
        d_flags|=dFL_USBSER;
        d_update();
      }
      return 1;
    } else if (!sertostart) {
      sertostart = 1;
      Serial.end();
      d_flags&=~dFL_USBSER;
      d_update();
    }
    return 0;
}

void serial_print_adc()
{
  for (int i = 0; i < 8; i++)  {
        Serial.print(adc_read(i));
        if (i == 7) Serial.println();
        else Serial.print(' ');
  }
}

void serial_print_rpt()
{
  Serial.print(rpt_data.buttons,HEX);Serial.print(' ');
  Serial.print(rpt_data.leftX);Serial.print(' ');
  Serial.print(rpt_data.leftY);Serial.print(' ');
  Serial.print(rpt_data.rightX);Serial.print(' ');
  Serial.println(rpt_data.rightY);
}

void serial_print_sym()
{
  Serial.print(txS.throttle);Serial.print(' ');
  Serial.print(txS.rudder);Serial.print(' ');
  Serial.print(txS.elevator);Serial.print(' ');
  Serial.println(txS.aileron);
}

void serial_cmd()
{
  if (Serial.available()==0) return;
  switch(Serial.read()) {
  case 'b':
    txstate=STATE_DOBIND;
    Serial.println("bind");
    break;
  case 'k':
    Serial.print("keys:0x");
    Serial.println(kscan_mx(),HEX);
    break;
  case 'x':
    Serial.print("sticks ");
    serial_print_adc();
    break;
  }
}

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

void menu_command(uint16_t k, uint16_t ch)
{
  void (**cmdlst)();

  d_update();
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
    uint8_t m=(i<2)?(i==0?KEY_1:KEY_2):(i==2?KEY_3:KEY_4);
    if (ch&k&m && cmdlst!=NULL && cmdlst[i]!=NULL)
      (cmdlst[i])();
  }
}

void setup()
{
  uint32_t t;

  // usb setup
  mygamepad_init();
  memset(&rpt_data, 0, sizeof(rpt_data));

  // tx general setup
  random_init();
  randomSeed(random_value());  
  MProtocol_id_master=random_id(EEPROM_ID_OFFSET,false);

  // protocols setup
  set_protocol(NULL);
  memset(&txB,0,sizeof(txB));
  BAYANG_TX_data(&txB);
  BAYANG_TX_id(MProtocol_id_master);
  memset(&txS,0,sizeof(txS));
  symax_tx_id(MProtocol_id_master);
  symax_data(&txS);

  // input setup
  kscan_init();
  adc_setup();

  // display setup
  u8g.setColorIndex(1);
  d_page = 1;
  d_flags |= dFL_PROTO_SYMAX;
  d_update();

  // timer setup
  t = micros();
  txtimer.start(t); txtimer.set(1000);
  potimer.start(t); potimer.set(1000);
  prtimer.start(t); prtimer.set(100000L);
}

void loop()
{
  uint32_t t = micros();
  uint16_t k_chg=0;
  if (txtimer.check(t)) {
        //readtest();
        txproto->data();
        if (txstate==STATE_DOINIT) txproto->init();
        if (txstate==STATE_DOBIND) txproto->bind();
        txtimer.set(txproto->callback());
        int8_t st=txproto->state();
        if (txstate!=st) {
          txstate=st;
          //Serial.println(txstate);
          d_update();
        }
  }

  if(potimer.check(t)) {
    kscan_tick();
    comm_usb();
    if (adc_scanall()) send_report();
//    if (rpt_test()) send_report();
    uint16_t k = kscan_keys();
    k_chg=(k^keys) & KEY_SYS;
    keys=k;
  }

  if (k_chg) menu_command(keys, k_chg);

  if(prtimer.check(t) &&comm_serial()) {
      serial_cmd();
      //serial_print_adc();
      //serial_print_rpt();
      //serial_print_sym();
  }

  if (d_updating) {
    d_updating = 0;
    u8g.firstPage();
    do draw(); while (u8g.nextPage());
  }
}
