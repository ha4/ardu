// moving to https://github.com/arduino/Arduino/wiki/PluggableUSB-and-PluggableHID-howto

#include <HID.h>
#include "util.h"
#include "Bayang_nrf24l01.h"

// display sh1106   GRN,SCL=PD0(d3,scl) BRN,SDA=PD1(d2,sda) BLU,GND YLW,VDD=+3.3
#include "U8glib.h"
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_DEV_0 | U8G_I2C_OPT_FAST); // Dev 0, Fast I2C / TWI


struct {
  uint8_t buttons;
  uint16_t leftX;
  uint16_t leftY;
  uint16_t rightX;
  uint16_t rightY;
} rpt_data; // 9 bytes

// 74hc164 pin 1&2 - DATA, 8 - CLOCK, pin 9 - CLEAR (+Vdd)
#define DATAPIN 8
#define CLKPIN  6
#define COLPINA 5
#define COLPINB 7

// nrf24l0 pin YLW,CS=PB0(ss) ORA,CE=PD4(d4) GRN,SCK=PB1 BRN,MOSI=PB2 BLK,MISO=PB3 RED,VDD=+3.3 BLU,GND

const uint8_t _hid_rpt_descr[] PROGMEM = {
  0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
  0x09, 0x05,                    // USAGE (Game Pad)
  0xa1, 0x01,                    // COLLECTION (Application)
  0xa1, 0x00,                    //   COLLECTION (Physical)
  0x85, 0x03,                	   //     REPORT_ID (3)

  0x05, 0x09,                    //     USAGE_PAGE (Button)
  0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
  0x29, 0x08,                    //     USAGE_MAXIMUM (Button 8)
  0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
  0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
  0x75, 0x01,                    //     REPORT_SIZE (1)
  0x95, 0x08,                    //     REPORT_COUNT (8)
  0x81, 0x02,                    //     INPUT (Data,Var,Abs)

  0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
  0x09, 0x30,                    //     USAGE (X)
  0x09, 0x31,                    //     USAGE (Y)
  0x09, 0x33,                    //     USAGE (Rx)
  0x09, 0x34,                    //     USAGE (Ry)
  0x35, 0x00,                    //     PHYSICAL_MINIMUM (0)
  0x46, 0xff, 0x03,              //     PHYSICAL_MAXIMUM (1023)
  0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
  0x26, 0xff, 0x03,              //     LOGICAL_MAXIMUM (1023)
  0x75, 0x10,                    //     REPORT_SIZE (16)
  0x95, 0x04,                    //     REPORT_COUNT (4)
  0x81, 0x02,                    //     INPUT (Data,Var,Abs)

  0xc0,                          //     END_COLLECTION
  0xc0                           // END_COLLECTION
};


#if !defined(_USING_HID)

#include "USBAPI.h"
#include "USBDesc.h"

#warning "Using legacy HID core (non pluggable)"
extern const HIDDescriptor _hid_iface PROGMEM;
const HIDDescriptor _hid_iface =
{
  D_INTERFACE(HID_INTERFACE, 1, 3, 0, 0), //n, endpoints, class, subclass,proto
  D_HIDREPORT(sizeof(_hid_rpt_descr)),
  // addr,attr,_packetsize,interval
  D_ENDPOINT(USB_ENDPOINT_IN (HID_ENDPOINT_INT), USB_ENDPOINT_TYPE_INTERRUPT, 64, 1)
};


u8 _hid_proto = 1;
u8 _hid_idlex = 1;


int HID_GetInterface(u8* interfaceNum)
{
  interfaceNum[0] += 1; // uses 1
  return USB_SendControl(TRANSFER_PGM, &_hid_iface, sizeof(_hid_iface));
}


int HID_GetDescriptor(int i)
{
  return USB_SendControl(TRANSFER_PGM, _hid_rpt_descr, sizeof(_hid_rpt_descr));
}

bool HID_Setup(Setup& setup)
{
  u8 r = setup.bRequest;
  if (setup.bmRequestType == REQUEST_DEVICETOHOST_CLASS_INTERFACE) {
    if (HID_GET_REPORT == r) {
      /*HID_GetReport();*/ return true;
    }
    if (HID_GET_PROTOCOL == r) {
      /*Send8(_hid_protocol); // TODO */ return true;
    }
  }
  else if (setup.bmRequestType == REQUEST_HOSTTODEVICE_CLASS_INTERFACE) {
    if (HID_SET_PROTOCOL == r) {
      _hid_proto = setup.wValueL;
      return true;
    }
    if (HID_SET_IDLE == r) {
      _hid_idlex = setup.wValueL;
      return true;
    }
  } else
    return false;
}

void send_report()
{
  uint8_t id = 3;
  USB_Send(HID_TX, &id, 1);
  USB_Send(HID_TX | TRANSFER_RELEASE, &rpt_data, sizeof(rpt_data));
}

#else

void mygamepad_init(void) {
  static HIDSubDescriptor node(_hid_rpt_descr, sizeof(_hid_rpt_descr));
  HID().AppendDescriptor(&node);
}

//mygamepad_ mygamepad;

void send_report()
{
  HID().SendReport(3, &rpt_data, sizeof(rpt_data));
}

#endif

#define POT_THROTTLE 2
#define POT_RUDDER   0
#define POT_ELEVATOR 1
#define POT_AILERON  3

byte adc_chan, adc_next;
volatile uint16_t adc_values[4];

ISR(ADC_vect)
{
#if defined(__AVR_ATmega32U4__)
  adc_values[adc_chan & 0x3] = ADC;
  ADMUX = adc_next;
  adc_chan = adc_next;
  adc_next = ((adc_next + 1) & 0x3) | 0x04 | _BV(REFS0);
#endif
}

uint16_t adc_read(uint8_t chan)
{
  uint16_t v;
  noInterrupts();
  v = adc_values[((~chan) + 1) & 0x3];
  interrupts();
  return v;
}

void adc_setup()
{
  // 125khz (ADPS=0b111, 16MHz div128)
  noInterrupts();
  adc_chan = adc_next = 0x04 | _BV(REFS0);
  ADMUX  = adc_chan;
  ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADATE) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
  ADCSRB = 0;
  DIDR0  = 0xF0; // digital input disable channel 4..7
  interrupts();
}

volatile uint16_t k_cnt;
volatile uint16_t k_matrix;

void kscan_init()
{
  k_cnt = 0;
  k_matrix = 0;
  pinMode(DATAPIN, OUTPUT);
  pinMode(CLKPIN, OUTPUT);
  pinMode(COLPINA, INPUT_PULLUP);
  pinMode(COLPINB, INPUT_PULLUP);
}

void kscan_tick()
{
  if (k_cnt == 0) k_cnt = 1;
  digitalWrite(CLKPIN, 0);
  digitalWrite(DATAPIN, (k_cnt == 1) ? 0 : 1);
  digitalWrite(CLKPIN, 1);
  delayMicroseconds(2);
  if (digitalRead(COLPINA)) k_matrix &= ~k_cnt; else k_matrix |= k_cnt; k_cnt <<= 1;
  if (digitalRead(COLPINB)) k_matrix &= ~k_cnt; else k_matrix |= k_cnt; k_cnt <<= 1;
}

uint16_t kscan_mx()
{
  do kscan_tick(); while (k_cnt != 0);
  return k_matrix;
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

#define dFL_USBSER 0x0001

volatile uint8_t d_updating, d_page;
volatile uint16_t d_flags;


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
      break;
  }
}

uint32_t t_scan, t_print;

void setup()
{
#if defined(_USING_HID)
  mygamepad_init();
#endif

  kscan_init();
  adc_setup();
  // display setup
  u8g.setColorIndex(1);
  d_page = 1;
  d_update();
  memset(&rpt_data, 0, sizeof(rpt_data));
  t_scan = micros();
  t_print = t_scan;
  rpt_data.buttons = 0x00;
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
  for (int i = 0; i < 4; i++)  {
        Serial.print(adc_read(i));
        if (i == 3) Serial.println();
        else Serial.print(' ');
  }
}

void loop()
{
  uint32_t t = micros();
  if (t - t_scan >= 1000) { // 1.0ms
    t_scan = t;
    if (adc_scanall()) send_report();
  }
  if (t - t_print >= 100000L) { // 0.1s
    t_print = t;
    if (comm_serial()) serial_print_adc();
  }

  if (d_updating) {
    d_updating = 0;
    u8g.firstPage();
    do draw(); while (u8g.nextPage());
  }

}
