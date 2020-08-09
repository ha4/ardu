// moving to https://github.com/arduino/Arduino/wiki/PluggableUSB-and-PluggableHID-howto

#include <HID.h>

//#define USE_SERIALSTRING
//#define USE_PPM
#define USE_SBUS

#ifdef USE_SBUS
#define dflt_max    1792
#define dflt_middle 992
#define dflt_min    192
#define dflt_ibase  1984
#else
#define dflt_max    2000
#define dflt_middle 1500
#define dflt_min    1000
#define dflt_ibase  3000
#endif

struct {
  uint8_t buttons;
  uint16_t rotY;
  uint16_t rotX;
  uint16_t slider;
  uint16_t rotZ;
} rpt_data; // 9 bytes

uint32_t t0;

// int0 = D3 in leonardo, int0=D2 in UNO, etc
#define pinINT0 3

/*
 * USB HID part
 */
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
                       
			0x09, 0x30,                    //     USAGE (x)     A
      0x09, 0x31,                    //     USAGE (y)     E
      0x09, 0x33,                    //     USAGE (Rx) T
      0x09, 0x34,                    //     USAGE (Ry)     R
#ifdef USE_SBUS
      0x16, 0xc0, 0x00,              //     LOGICAL_MINIMUM (192)
      0x26, 0x00, 0x07,              //     LOGICAL_MAXIMUM (1792)
#else
      0x16, 0xe8, 0x03,              //     LOGICAL_MINIMUM (1000)
      0x26, 0xd0, 0x07,              //     LOGICAL_MAXIMUM (2000)
#endif
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
 * PPM input part
 */
#define PPM_number 8
int ppm[PPM_number];
byte PPM_chan;
volatile byte PPM_state;

void ISR1()
{
  static uint16_t old = 0;
  uint16_t cur, pulse;
//  cur = TCNT1;
  cur = micros();
  pulse = cur;
  pulse -= old;
  old = cur;
//  pulse >>=1; // div 2

  if (pulse > 3000) {
    PPM_chan = 0;
    PPM_state= 1;
    return;
  }
  if (pulse < 500) return; // skip it
  if (pulse < dflt_min) pulse = dflt_min;
  if (pulse > dflt_max) pulse = dflt_max;
  ppm[PPM_chan] = pulse;
  if (PPM_chan < PPM_number-1) PPM_chan++; //else PPM_chan=0;
}

void ppm_input_start()
{
  cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;
  TCNT1 = 0;
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
  PPM_chan = 0;
  PPM_state = 0;
  sei();
  attachInterrupt(digitalPinToInterrupt(pinINT0), ISR1, FALLING);   
}

bool scanppm()
{
  if (!PPM_state) return false;
  PPM_state=0;
  rpt_data.rotY=dflt_ibase-ppm[0];
  rpt_data.rotX=ppm[1];
  rpt_data.slider=ppm[2];
  rpt_data.rotZ=dflt_ibase-ppm[3];
  if (ppm[4] > dflt_middle) rpt_data.buttons |= 0x01; else rpt_data.buttons &= ~0x01;
  if (ppm[5] > dflt_middle) rpt_data.buttons |= 0x02; else rpt_data.buttons &= ~0x02;
  if (ppm[6] > dflt_middle) rpt_data.buttons |= 0x04; else rpt_data.buttons &= ~0x04;
  if (ppm[7] > dflt_middle) rpt_data.buttons |= 0x08; else rpt_data.buttons &= ~0x08;
  return true;
}

/*
 * SBUS input part
 */
#define SBUS_TIME_NEEDED_PER_FRAME    3000

#define SBUS_FLAG_SIGNAL_LOSS       (1 << 2)
#define SBUS_FLAG_FAILSAFE_ACTIVE   (1 << 3)

typedef struct sbusChannels_s {
    // 176 bits of data (11 bits per channel * 16 channels) = 22 bytes.
    unsigned int chan0 : 11;
    unsigned int chan1 : 11;
    unsigned int chan2 : 11;
    unsigned int chan3 : 11;
    unsigned int chan4 : 11;
    unsigned int chan5 : 11;
    unsigned int chan6 : 11;
    unsigned int chan7 : 11;
    unsigned int chan8 : 11;
    unsigned int chan9 : 11;
    unsigned int chan10 : 11;
    unsigned int chan11 : 11;
    unsigned int chan12 : 11;
    unsigned int chan13 : 11;
    unsigned int chan14 : 11;
    unsigned int chan15 : 11;
    uint8_t flags;
} __attribute__((__packed__)) sbusChannels_t;

sbusChannels_t sbus;

int8_t SBUS_ptr;
uint32_t SBUS_tfrm;

void sbus_input_init()
{
  Serial1.begin(100000,SERIAL_8E2);
  while(!Serial1);
  SBUS_ptr = -1;
  SBUS_tfrm = 0;
}

bool sbus_decode()
{
  rpt_data.rotY=dflt_ibase-sbus.chan0;
  rpt_data.rotX=sbus.chan1;
  rpt_data.slider=sbus.chan2;
  rpt_data.rotZ=dflt_ibase-sbus.chan3;
  if (sbus.chan4 > dflt_middle) rpt_data.buttons |= 0x01; else rpt_data.buttons &= ~0x01;
  if (sbus.chan5 > dflt_middle) rpt_data.buttons |= 0x02; else rpt_data.buttons &= ~0x02;
  if (sbus.chan6 > dflt_middle) rpt_data.buttons |= 0x04; else rpt_data.buttons &= ~0x04;
  if (sbus.chan7 > dflt_middle) rpt_data.buttons |= 0x08; else rpt_data.buttons &= ~0x08;
  return true;
}

bool scansbus(uint32_t t)
{
  uint8_t *data=(uint8_t*)&sbus;
  uint32_t dt; 
  if (!Serial1.available()) return false;

  dt = t - SBUS_tfrm;
  if (dt > (SBUS_TIME_NEEDED_PER_FRAME+500)) SBUS_ptr = -1;

  while(Serial1.available()) {
    uint8_t c = Serial1.read();
    if (SBUS_ptr == -1) {
      if (c == 0x0F) { SBUS_tfrm = t; SBUS_ptr++; }
      continue;
    }
    if (SBUS_ptr >= sizeof(sbus)) continue;
    data[SBUS_ptr++]=c;
    if (SBUS_ptr == sizeof(sbus)) return sbus_decode();
  }
  return false;
}

/*
 * test
 */
bool scantest()
{
  bool c = 0;
  static int dir=1;
  rpt_data.rotY+=dir;
  rpt_data.rotX+=dir;
  c=1;
  if (rpt_data.rotY >=2000) dir=-dir;
  if (rpt_data.rotY <=1000) dir=-dir;
  return c;
}

void setup()
{
  memset(&rpt_data,0,sizeof(rpt_data));
#ifdef USE_PPM
  pinMode(pinINT0,INPUT_PULLUP);
  ppm_input_start();
#endif
  t0=0;
  // release hat
  rpt_data.buttons = 0x00;
  rpt_data.rotX=dflt_middle;
  rpt_data.rotY=dflt_middle;
  rpt_data.rotZ=dflt_middle;
  rpt_data.slider=dflt_min;
  
#if defined(_USING_HID)
mygamepad_init();
#endif

#ifdef USE_SBUS
  sbus_input_init();
#endif

#ifdef USE_SERIALSTRING
  Serial.begin(115200);
  while(!Serial);
  Serial.println("Receiver");
#endif
}

void loop()
{
  uint32_t t=micros();
  if(t-t0 < 1000) return;
  t0=t;
#ifdef USE_SBUS
  if (scansbus(t)) 
#else
  if (scanppm()) 
#endif
  {
    send_report(); 
#ifdef USE_SERIALSTRING
    Serial.print(rpt_data.buttons,HEX); Serial.print(' ');
    Serial.print(rpt_data.rotY); Serial.print(' ');
    Serial.print(rpt_data.rotX); Serial.print(' ');
    Serial.print(rpt_data.slider); Serial.print(' ');
    Serial.print(rpt_data.rotZ);
    Serial.println();
#endif
  }
}
