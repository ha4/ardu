#include "usb_gpad.h"
#include "tx_util.h"
#include "Bayang_nrf24l01.h"
#include "symax_nrf24l01.h"
#include "xtimer.h"
#include "inp_x.h"
#include "visual.h"

// nrf24l0 pin YLW,CS=PB0(ss,d17) ORA,CE=PD4(d4) GRN,SCK=PB1(d15) BRN,MOSI=PB2(d16) BLK,MISO=PB3(d14) RED,VDD=+3.3 BLU,GND
// display sh1106   GRN,SCL=PD0(d3,scl) BRN,SDA=PD1(d2,sda) BLU,GND YLW,VDD=+3.3
// key-rows 74hc164 pin 1&2 = DATA(d8), 8 - CLOCK(d6), pin 9 - CLEAR (+Vdd)
// key-columns pullup=10k COL(A)=(d5), COL(B)=(d7)

#define EEPROM_ID_OFFSET    10    // Module ID (4 bytes)

void none_data();
void bayang_data();
void symax_data();

struct SymaXData txS;
struct BayangData txB;

struct proto_t tx_lst[] = {
  {&noneTX_init, &noneTX_bind, &noneTX_callback, &noneTX_state, &none_data, "NoTxm" },
  {&BAYANG_TX_init, &BAYANG_TX_bind, &BAYANG_TX_callback, &BAYANG_TX_state, &bayang_data, "Bayng" },
  {&symax_init, &symax_bind, &symax_callback, &symax_state, &symax_data, "SymaX" }, 
  {NULL, NULL, NULL, NULL, NULL, NULL}
};

xtimer txtimer; // Transmitter
xtimer potimer; // Pot read adc
xtimer prtimer; // serial print

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
  uint16_t elevt = adc_read(POT_ELEVATOR);
  uint16_t ailrn = adc_read(POT_AILERON);
  uint16_t throt = adc_read(POT_THROTTLE);
  uint16_t rudd  = adc_read(POT_RUDDER);

  txB.roll=ailrn;
  txB.pitch=1023-elevt;
  txB.throttle=1023-throt;
  txB.yaw=1023-rudd;
  txS.flags5=FLAG5_HIRATE;
  
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
  txS.throttle=(1023-throt)/4-48;
  txS.rudder=(512-rudd)/4;
}


void comm_usb()
{ /* ###FIXME: not-functional now */
   if(USBSTA&(1<<VBUS)) {
    if(d_flags&dFL_VUSB) return;
    d_flags|=dFL_VUSB;
    v_update();
   } else {
    if(!(d_flags&dFL_VUSB)) return;
    d_flags&=~dFL_VUSB;
    v_update();
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
        v_update();
      }
      return 1;
    } else if (!sertostart) {
      sertostart = 1;
      Serial.end();
      d_flags&=~dFL_USBSER;
      v_update();
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

  // visual setup
  v_init();

  // timers setup
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
          v_update();
        }
  }

  if(potimer.check(t)) {
    kscan_tick();
    comm_usb();
    if (adc_scanall()) send_report();
//    if (rpt_test()) send_report();

    uint16_t k = kscan_keys();
    k_chg=(k^v_keys) & KEY_SYS;
    v_keys=k;
  }

  if (k_chg) menu_command(v_keys, k_chg);

  if(prtimer.check(t) &&comm_serial()) {
      serial_cmd();
      //serial_print_adc();
      //serial_print_rpt();
      //serial_print_sym();
  }

  v_loop();
}
