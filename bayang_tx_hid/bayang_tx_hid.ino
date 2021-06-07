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

void bayang_data();
void symax_data();

struct proto_t {
  uint8_t proto_id;
  uint16_t (*tx_id)(uint32_t id);
  uint16_t (*init)();
  uint16_t (*bind)();
  uint16_t (*callback)();
  int8_t (*state)();
  void (*load_data)();
};

struct SymaXData txS;
struct BayangData txB;
struct proto_t tx_lst[] = {
  {1, &BAYANG_TX_id, &BAYANG_TX_init, &BAYANG_TX_bind, &BAYANG_TX_callback, &BAYANG_TX_state, &bayang_data },
  {2, &symax_tx_id, &symax_init, &symax_bind, &symax_callback, &symax_state, &symax_data },  
};

xtimer txtimer; // Transmitter
xtimer potimer; // Pot read adc
xtimer prtimer; // serial print

volatile uint8_t d_updating, d_page;
volatile uint16_t d_flags;

uint32_t MProtocol_id_master;
int txstate=100;

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

  uint8_t b = k_matrix;
  int16_t ry = adc_read(POT_ELEVATOR);
  int16_t rx = adc_read(POT_AILERON);
  int16_t ly = adc_read(POT_THROTTLE);
  int16_t lx = adc_read(POT_RUDDER);
  if (b != rpt_data.buttons || 
      ry != rpt_data.rightY || rx != rpt_data.rightX ||
      ly != rpt_data.leftY || lx != rpt_data.leftX) {
    rpt_data.leftX = lx-512;
    rpt_data.leftY = ly-512;
    rpt_data.rightX = -(rx-512);
    rpt_data.rightY = ry-512;
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

void bayang_data()
{
  
}

void symax_data()
{
  uint8_t b = k_matrix;
  uint16_t elevt = adc_read(POT_ELEVATOR);
  uint16_t ailrn = adc_read(POT_AILERON);
  uint16_t throt = adc_read(POT_THROTTLE);
  uint16_t rudd  = adc_read(POT_RUDDER);

  txS.aileron=(ailrn-512)/4;
  txS.elevator=(512-elevt)/4;
  txS.throttle=(1024-throt)/4-34;
  txS.rudder=(512-rudd)/4;
  txS.flags5=FLAG5_HIRATE;
}


void d_update()
{
  d_updating = 1;
}

void draw(void)
{
  switch (d_page) {
    case 0:
      u8g.setFont(u8g_font_unifont); //(u8g_font_osb21);
      u8g.drawStr( 0, 22, "Hello World!");
      break;
    case 1:
      u8g.setFont(u8g_font_unifont); //(u8g_font_osb21);
      u8g.drawStr( 76, 12, (d_flags&dFL_VUSB)?((d_flags&dFL_USBSER)? "SERIAL":"USB"):"no usb");
      u8g.drawStr( 76, 24, (txstate)? "bound":"bind ");
      u8g.drawStr( 76, 36, (d_flags&dFL_PROTO_SYMAX)? "SymaX":"Bayang");
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

void serial_cmd()
{
  if (Serial.available()==0) return;
  switch(Serial.read()) {
  case 'b':
    //txtimer.set(BAYANG_TX_bind());
    txtimer.set(symax_bind());
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
  d_flags |= dFL_PROTO_SYMAX;
  d_update();
  memset(&rpt_data, 0, sizeof(rpt_data));

  t = micros();
  txtimer.start(t);
  potimer.start(t); potimer.set(1000);
  prtimer.start(t); prtimer.set(100000L);
  
  BAYANG_TX_data(&txB);
  memset(&txB,0,sizeof(txB));
  symax_data(&txS);
  memset(&txS,0,sizeof(txS));

//  BAYANG_TX_id(MProtocol_id_master);
//  txtimer.set(BAYANG_TX_init());
//  BAYANG_TX_bind(); // autobind

  symax_tx_id(0x7F7FC0D7ul);
  txtimer.set(symax_init());
}

void loop()
{
  uint32_t t = micros();
  if (txtimer.check(t)) {
        //readtest();
        symax_data();
        //txtimer.set(BAYANG_TX_callback());
        txtimer.set(symax_callback());
        //if (txstate!=BAYANG_TX_state()) { txstate=BAYANG_TX_state(); Serial.println(txstate); }
        if (txstate!=symax_state()) { txstate=symax_state(); Serial.println(txstate); }
  }

  if(potimer.check(t)) {
    comm_usb();
    kscan_tick();
    if (adc_scanall()) send_report();
//    if (rpt_test()) send_report();
  }

  if(prtimer.check(t))
    if (comm_serial()) {
      serial_cmd();
      serial_print_adc();
      //serial_print_rpt();
    }

  if (d_updating) {
    d_updating = 0;
    u8g.firstPage();
    do draw(); while (u8g.nextPage());
  }
}
