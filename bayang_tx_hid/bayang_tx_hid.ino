// moving to https://github.com/arduino/Arduino/wiki/PluggableUSB-and-PluggableHID-howto

#include <HID.h>

struct {
  uint16_t buttons;
  int8_t leftX;
  int8_t leftY;
  int8_t rightX;
  int8_t rightY;
} rpt_data; // 6 bytes

uint32_t t0;

// 74hc164 pin 1&2 - DATA
#define DATAPIN 4
// 74hc164 pin 8 - CLOCK, pin 9 - CLEAR (+Vcc)
#define CLKPIN  3
#define COLPINA 5
#define COLPINB 2

#define RY0 470
#define RX0 528
#define LY0 517
#define LX0 481
#define RYM 439
#define RXM 445
#define LYM 256
#define LXM 406

// power(2,20)/MAX
#define REVM(a) (1048576L/a)
#define MAXM    (1048576L)
#define MAXS    (20-7)

const uint8_t _hid_rpt_descr[] PROGMEM = {
			0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
			0x09, 0x05,                    // USAGE (Game Pad)
			0xa1, 0x01,                    // COLLECTION (Application)
			0xa1, 0x00,                    //   COLLECTION (Physical)
			0x85, 0x03,                	   //     REPORT_ID (3)

			0x05, 0x09,                    //     USAGE_PAGE (Button)
			0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
			0x29, 0x10,                    //     USAGE_MAXIMUM (Button 16)
			0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
			0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
			0x75, 0x01,                    //     REPORT_SIZE (1)
			0x95, 0x0c,                    //     REPORT_COUNT (12)
			0x81, 0x02,                    //     INPUT (Data,Var,Abs)

			0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
		        0x09, 0x39,                    // USAGE (Hat Switch)
		        0x15, 0x00,                    // LOGICAL_MINIMUM (0)
                        0x25, 0x07,                    // LOGICAL_MAXIMUM (7)   // if 8, hat switch is released
                        0x35, 0x00,                    // PHYSICAL_MINIMUM (0)
                        0x46, 0x3B, 0x01,              // PHYSICAL_MAXIMUM (315)
                        0x65, 0x14,                    // UNIT (Eng Rot:Angular Pos)
                        0x75, 0x04,                    // REPORT_SIZE (4)
                        0x95, 0x01,                    // REPORT_COUNT (1)
                        0x81, 0x02,                    // INPUT (Data,Var,Abs)
                        
			0x09, 0x30,                    //     USAGE (X)
			0x09, 0x31,                    //     USAGE (Y)
			0x09, 0x33,                    //     USAGE (Rx)
			0x09, 0x34,                    //     USAGE (Ry)
			0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
			0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
			0x75, 0x08,                    //     REPORT_SIZE (8)
			0x95, 0x04,                    //     REPORT_COUNT (4)
			0x81, 0x02,                    //     INPUT (Data,Var,Abs)

			0xc0,                          //     END_COLLECTION
			0xc0                           // END_COLLECTION
};

#if 0
	0x05, 0x01,         /*          Usage Page (Desktop),       */
	0x75, 0x10,         /*          Report Size (16),           */
	0x46, 0xFF, 0xFF,   /*          Physical Maximum (65535),   */
	0x27, 0xFF, 0xFF, 0x00, 0x00, /*      Logical Maximum (65535),    */
	0x95, 0x03,         /*          Report Count (3),           * 3x Accels */
	0x09, 0x33,         /*              Usage (rX),             */
	0x09, 0x34,         /*              Usage (rY),             */
	0x09, 0x35,         /*              Usage (rZ),             */
	0x81, 0x02,         /*          Input (Variable),           */
	0x06, 0x00, 0xFF,   /*          Usage Page (FF00h),         */
	0x95, 0x03,         /*          Report Count (3),           * Skip Accels 2nd frame */
	0x81, 0x02,         /*          Input (Variable),           */
	0x05, 0x01,         /*          Usage Page (Desktop),       */
	0x09, 0x01,         /*          Usage (Pointer),            */
	0x95, 0x03,         /*          Report Count (3),           * 3x Gyros */
	0x81, 0x02,         /*          Input (Variable),           */
	0x06, 0x00, 0xFF,   /*          Usage Page (FF00h),         */
	0x95, 0x03,         /*          Report Count (3),           * Skip Gyros 2nd frame */
	0x81, 0x02, /*          Input (Variable),           */
#endif


#if !defined(_USING_HID)

#include "USBAPI.h"
#include "USBDesc.h"

#warning "Using legacy HID core (non pluggable)"
extern const HIDDescriptor _hid_iface PROGMEM;
const HIDDescriptor _hid_iface =
{
  D_INTERFACE(HID_INTERFACE,1,3,0,0),  //n, endpoints, class, subclass,proto
  D_HIDREPORT(sizeof(_hid_rpt_descr)),
        // addr,attr,_packetsize,interval
  D_ENDPOINT(USB_ENDPOINT_IN (HID_ENDPOINT_INT),USB_ENDPOINT_TYPE_INTERRUPT,64,1)
};


u8 _hid_proto = 1;
u8 _hid_idlex = 1;


int HID_GetInterface(u8* interfaceNum)
{
  interfaceNum[0] += 1; // uses 1
  return USB_SendControl(TRANSFER_PGM,&_hid_iface,sizeof(_hid_iface));
}


int HID_GetDescriptor(int i)
{
  return USB_SendControl(TRANSFER_PGM,_hid_rpt_descr,sizeof(_hid_rpt_descr));
}

bool HID_Setup(Setup& setup)
{
  u8 r = setup.bRequest;
  if (setup.bmRequestType == REQUEST_DEVICETOHOST_CLASS_INTERFACE) {
    if (HID_GET_REPORT == r) { /*HID_GetReport();*/ return true; }
    if (HID_GET_PROTOCOL == r) {/*Send8(_hid_protocol); // TODO */ return true; }
  }
        else if (setup.bmRequestType == REQUEST_HOSTTODEVICE_CLASS_INTERFACE) {
    if (HID_SET_PROTOCOL == r) { _hid_proto = setup.wValueL; return true; }
    if (HID_SET_IDLE == r) { _hid_idlex = setup.wValueL; return true; }
  } else
              return false;
}

void send_report()
{
  uint8_t id=3;
  USB_Send(HID_TX, &id, 1);
  USB_Send(HID_TX | TRANSFER_RELEASE,&rpt_data,sizeof(rpt_data));
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


/*
L   0008 L2  0800 R2   0004 R    0400
Hu  0020 Hl  0010 Hr   2000 Hd   1000
MIN 0080 PLS 8000 Lstk 0040 Rstk 4000
X   0200 Y   0002 A    0100 B    0001
LX A3 LY A2 RX A1 RY A0
*/

uint16_t permb(uint16_t in)
{ /// 4000,8000 <-> 0020,0010
  uint16_t perm1 = (in & 0x0030) << 10;
  uint16_t perm2 = (in >> 10) & 0x0030;
  return (in & ~0xC030)|perm1|perm2;
}

/*  after permutation: ^8000 >2000 v1000 <4000
  7 0 1
 6     2
  5 4 3
*/
uint16_t hatb(uint16_t b)
{
  switch(b&0xF000) {
  case 0x8000: return 0x0000;
  case 0xC000: return 0x7000;
  case 0xA000: return 0x1000;
  case 0x2000: return 0x2000;
  case 0x1000: return 0x4000;
  case 0x3000: return 0x3000;
  case 0x5000: return 0x5000;
  case 0x4000: return 0x6000;
  default: return 0x8000;
  }
}

uint16_t permhat(uint16_t in)
{
  return (in & 0xFFF) | hatb(in);
}

void scan_init()
{
  pinMode(DATAPIN, OUTPUT);
  pinMode(CLKPIN, OUTPUT);
  pinMode(COLPINA, INPUT_PULLUP);
  pinMode(COLPINB, INPUT_PULLUP);
}

uint16_t scanmx()
{
  uint8_t cnt, pa, pb;
  for(cnt=pa=pb=0; cnt < 8; cnt++) {
    digitalWrite(CLKPIN, 0);
    digitalWrite(DATAPIN,(cnt==0)?0:1);
    digitalWrite(CLKPIN, 1);
    delayMicroseconds(2);
    pa=(pa<<1) | (digitalRead(COLPINA)?0:1);
    pb=(pb<<1) | (digitalRead(COLPINB)?0:1);
  }
  return (pa << 8) | pb;
}

int8_t cart(int16_t a, int16_t offs, int32_t scale)
{
  int z = a-offs;
  int32_t z2 = z*scale;
  int8_t p;
  if (z2 >= MAXM) z2=MAXM-1;
  if (z2 <= -MAXM) z2=-MAXM;
  p = z2 >> MAXS;
  return p;
}

bool scanall()
{
  bool c = 0;
  
  uint16_t b=permhat(permb(scanmx()));
  int8_t ry=cart(analogRead(A0),RY0,REVM(RYM));
  int8_t rx=cart(analogRead(A1),RX0,REVM(RXM));
  int8_t ly=cart(analogRead(A2),LY0,REVM(LYM));
  int8_t lx=cart(analogRead(A3),LX0,REVM(LXM));
  if (b!= rpt_data.buttons) { rpt_data.buttons=b; c=1; }
  if (ry!= rpt_data.rightY) { rpt_data.rightY=ry; c=1; }
  if (rx!= rpt_data.rightX) { rpt_data.rightX=rx; c=1; }
  if (ly!= rpt_data.leftY) { rpt_data.leftY=ly; c=1; }
  if (lx!= rpt_data.leftX) { rpt_data.leftX=lx; c=1; }
  
  return c;
}

bool scantest()
{
  bool c = 0;
  static int dir=1;
  rpt_data.rightY+=dir;
  rpt_data.rightX+=dir;
  c=1;
  if (rpt_data.rightY >=127) dir=-dir;
  if (rpt_data.rightY <=-127) dir=-dir;
  return c;
}

void setup0()
{
  scan_init();
  memset(&rpt_data,0,sizeof(rpt_data));
  t0=0;
  // release hat
  rpt_data.buttons = 0x8000;
#if defined(_USING_HID)
mygamepad_init();
#endif
}

void loop0()
{
  uint32_t t=millis();  
  if(t-t0 > 1) { t0=t; if (scanall()) send_report(); }
}

#define POT_THROTTLE 2
#define POT_RUDDER   0
#define POT_ELEVATOR 1
#define POT_AILERON  3
#define ADC0_RUDDER   7
#define ADC1_ELEVATOR 6
#define ADC2_THROTTLE 5
#define ADC3_AILERON  4

byte adc_chan;
volatile uint16_t adc_values[4];

ISR(ADC_vect)
{
#if defined(__AVR_ATmega32U4__)
  /* now prev idx Achan
   * 7   4    3   3
   * 6   7    2   0
   * 5   6    1   1
   * 4   5    0   2
   */
  ADMUX = adc_chan; // catch before sample hold
  uint16_t v = ADC;
  adc_values[adc_chan&0x3]=v;
  adc_chan--;
  adc_chan&=0x3;
  adc_chan|=_BV(REFS0) | 0x04;
#endif
}

uint16_t adc_read(uint8_t chan)
{
  /*  idx Achan ~achn ~achn-1
   *  3   3     0     3
   *  0   2     1     0
   *  1   1     2     1
   *  2   0     3     2   */
  uint8_t  x;
  uint16_t v;
  x=~chan;
//  x--;
  x&=0x3;
  noInterrupts();
  v = adc_values[x];
  interrupts();
  return v;
}

void adc_setup()
{
  // 125khz (ADPS=0b111, 16MHz div128)
  noInterrupts();
  adc_chan=0x07|_BV(REFS0);
  ADMUX  = adc_chan;
  ADCSRA = _BV(ADEN)|_BV(ADSC)|_BV(ADATE)|_BV(ADIE)|_BV(ADPS2)|_BV(ADPS1)|_BV(ADPS0);
  ADCSRB = 0;
  DIDR0  = 0xF0; // digital input disable channel 4..7
  interrupts();
}

uint16_t channels[4];

void getADC0()
{
  for(int i=0; i < 4; i++)
    channels[i]=analogRead(i);
}

void getADC1()
{
  for(int i=0; i < 4; i++)
    channels[i]=adc_read(i);
}

void printChans()
{
  for(int i=0; i < 4; i++) {
    Serial.print(channels[i]);
    if (i==3) Serial.println(); else Serial.print(' ');
  }
}

void setup()
{
  while(!Serial);
  Serial.begin(115200);
  adc_setup();
}

void loop()
{
  getADC1();
  printChans();
  delay(100);
}
