#include "usb_gpad.h"
#include "tx_util.h"
#include "Bayang_nrf24l01.h"
#include "symax_nrf24l01.h"
#include "xtimer.h"
#include "inp_x.h"

// display sh1106   GRN,SCL=PD0(d3,scl) BRN,SDA=PD1(d2,sda) BLU,GND YLW,VDD=+3.3
#include "U8glib.h"


// nrf24l0 pin YLW,CS=PB0(ss,d17) ORA,CE=PD4(d4) GRN,SCK=PB1(d15) BRN,MOSI=PB2(d16) BLK,MISO=PB3(d14) RED,VDD=+3.3 BLU,GND

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_DEV_0 | U8G_I2C_OPT_FAST); // Dev 0, Fast I2C / TWI


struct SymaXData txS;
struct BayangData txB;

xtimer txtimer; // Transmitter
xtimer potimer; // Pot read adc
xtimer prtimer; // serial print

#define EEPROM_ID_OFFSET    10    // Module ID (4 bytes)

#define dFL_USBSER 0x0001

volatile uint8_t d_updating, d_page;
volatile uint16_t d_flags;

uint32_t MProtocol_id_master;
int txstate=100;


int  limadd(int a, int b) { a+=b;  return max(-511,min(a,511)); }

void readtest()
{
    static int sa=0, ca=511, sb=511, cb=0;
    txS.aileron=sa/4;
    txS.elevator=cb/4;
    txS.throttle=(511+sb)>>2;
    txS.rudder=ca/4;

    int ts=sa/128,  tc=ca/128;
    sa=limadd(sa,tc), ca=limadd(ca,-ts);
    ts=sb/32,  tc=cb/32;
    sb=limadd(sb,tc), cb=limadd(cb,-ts);
}



bool adc_scanall()
{
  bool c = 0;

  uint8_t b = 0;
  uint16_t ry = adc_read(POT_ELEVATOR);
  uint16_t rx = adc_read(POT_AILERON);
  uint16_t ly = adc_read(POT_THROTTLE);
  uint16_t lx = adc_read(POT_RUDDER);
  if (b != rpt_data.buttons) {
    rpt_data.buttons = b;
    c = 1;
  }
  if (ry != rpt_data.rightY) {
    rpt_data.rightY = ry;
    c = 1;
  }
  if (rx != rpt_data.rightX) {
    rpt_data.rightX = rx;
    c = 1;
  }
  if (ly != rpt_data.leftY) {
    rpt_data.leftY = ly;
    c = 1;
  }
  if (lx != rpt_data.leftX) {
    rpt_data.leftX = lx;
    c = 1;
  }

  return c;
}


void d_update() {
  d_updating = 1;
}


void draw(void) {
  switch (d_page) {
    case 0:
      u8g.setFont(u8g_font_unifont); //(u8g_font_osb21);
      u8g.drawStr( 0, 22, "Hello World!");
      break;
    case 1:
      u8g.setFont(u8g_font_unifont); //(u8g_font_osb21);
      u8g.drawStr( 34, 22, (d_flags&dFL_USBSER)? "SERIAL":"no usb");
      u8g.drawStr(  0, 22, (txstate)? "bound":"bind ");
      break;
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

void serial_cmd()
{
  if (Serial.available()==0) return;
  switch(Serial.read()) {
  case 'b':
    //txtimer.set(BAYANG_TX_bind());
    txtimer.set(symax_bind());
    Serial.println("bind");
    break;
  }
}

void serial_print_adc()
{
  for (int i = 0; i < 4; i++)  {
        Serial.print(adc_read(i));
        if (i == 3) Serial.println();
        else Serial.print(' ');
  }
}

void setup()
{
  uint32_t t;

  mygamepad_init();

  random_init();
  randomSeed(random_value());  
  MProtocol_id_master=random_id(EEPROM_ID_OFFSET,false);


  kscan_init();
  adc_setup();

  // display setup
  u8g.setColorIndex(1);
  d_page = 1;
  d_update();
  memset(&rpt_data, 0, sizeof(rpt_data));

  t = micros();
  txtimer.start(t);
  potimer.start(t); potimer.set(1000);
  prtimer.start(t); prtimer.set(100000L);
  
  rpt_data.buttons = 0x00;

//  BAYANG_TX_id(MProtocol_id_master);
//  txtimer.set(BAYANG_TX_init());
//  BAYANG_TX_bind(); // autobind
//  BAYANG_TX_data(&txB);
  memset(&txB,0,sizeof(txB));

  symax_tx_id(0x7F7FC0D7ul);
  txtimer.set(symax_init());
  symax_data(&txS);
  memset(&txS,0,sizeof(txS));
}

void loop()
{
  uint32_t t = micros();
  if (txtimer.check(t)) {
        readtest();
        //txtimer.set(BAYANG_TX_callback());
        txtimer.set(symax_callback());
        //if (txstate!=BAYANG_TX_state()) { txstate=BAYANG_TX_state(); Serial.println(txstate); }
        if (txstate!=symax_state()) { txstate=symax_state(); Serial.println(txstate); }
  }

  if(potimer.check(t)) 
    if (adc_scanall()) send_report();

  if(prtimer.check(t))
    if (comm_serial()) {
      serial_cmd();
      //serial_print_adc();
    }

  if (0 && d_updating) {
    d_updating = 0;
    u8g.firstPage();
    do draw(); while (u8g.nextPage());
  }
}
