// moving to https://github.com/arduino/Arduino/wiki/PluggableUSB-and-PluggableHID-howto

#include <HID.h>

//#define USE_SERIALSTRING

// min 192  max 1720 span 1600 middle 992
#define sbus_scale  5
#define sbus_middle 992
#define sbus_min    192
//#define SBUS_to_HID(x) (((x) - sbus_min)*sbus_scale)
#define SBUS_to_HID(x) ((x)+((x)<<2) - sbus_min*sbus_scale)

// min 1000 max 2000 span 1000 middle 1500
#define ppm_middle 1500
#define ppm_scale  8
#define ppm_min 1000
#define ppm_max 2000
//#define PPM_to_HID(x) (((x) - ppm_min)*ppm_scale)
#define PPM_to_HID(x) (((x)<<3) - ppm_min*ppm_scale)

#define output_max 8000
#define output_min 0
#define I(x) (output_max-(x))

struct {
  uint8_t buttons;
  int16_t X;
  int16_t Y;
  int16_t Rx;
  int16_t Ry;
} rpt_data; // 9 bytes

uint32_t t0;

// int0 = D3 in leonardo, int0=D2 in UNO, etc
#define pinINT0 2

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
      0x09, 0x33,                    //     USAGE (Rx)    T
      0x09, 0x34,                    //     USAGE (Ry)    R
      0x16, 0x00, 0x00,              //     LOGICAL_MINIMUM (0)
      0x26, 0x40, 0x1F,              //     LOGICAL_MAXIMUM (8000)
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
    if (PPM_chan > 0) PPM_state= 1;
    PPM_chan = 0;
    return;
  }
  if (pulse < 500) return; // skip it
  if (pulse < ppm_min) pulse = ppm_min;
  if (pulse > ppm_max) pulse = ppm_max;
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
  rpt_data.X     =  PPM_to_HID(ppm[0]);
  rpt_data.Y     =  PPM_to_HID(ppm[1]);
  rpt_data.Rx    =  PPM_to_HID(ppm[2]);
  rpt_data.Ry    =I(PPM_to_HID(ppm[3]));
  if (ppm[4] > ppm_middle) rpt_data.buttons |= 0x01; else rpt_data.buttons &= ~0x01;
  if (ppm[5] > ppm_middle) rpt_data.buttons |= 0x02; else rpt_data.buttons &= ~0x02;
  if (ppm[6] > ppm_middle) rpt_data.buttons |= 0x04; else rpt_data.buttons &= ~0x04;
  if (ppm[7] > ppm_middle) rpt_data.buttons |= 0x08; else rpt_data.buttons &= ~0x08;
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
  rpt_data.X     =  SBUS_to_HID(sbus.chan0);
  rpt_data.Y     =  SBUS_to_HID(sbus.chan1);
  rpt_data.Rx    =  SBUS_to_HID(sbus.chan2);
  rpt_data.Ry    =I(SBUS_to_HID(sbus.chan3));
  if (sbus.chan4 > sbus_middle) rpt_data.buttons |= 0x01; else rpt_data.buttons &= ~0x01;
  if (sbus.chan5 > sbus_middle) rpt_data.buttons |= 0x02; else rpt_data.buttons &= ~0x02;
  if (sbus.chan6 > sbus_middle) rpt_data.buttons |= 0x04; else rpt_data.buttons &= ~0x04;
  if (sbus.chan7 > sbus_middle) rpt_data.buttons |= 0x08; else rpt_data.buttons &= ~0x08;
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
  rpt_data.X+=dir;
  rpt_data.Y+=dir;
  c=1;
  if (rpt_data.X >=8000) dir=-dir;
  if (rpt_data.X <=0) dir=-dir;
  return c;
}

void setup()
{
  memset(&rpt_data,0,sizeof(rpt_data));
  t0=0;

  pinMode(pinINT0,INPUT_PULLUP);
  ppm_input_start();
  
#if defined(_USING_HID)
  mygamepad_init();
#endif

  sbus_input_init();

#ifdef USE_SERIALSTRING
  Serial.begin(115200);
  while(!Serial);
  Serial.println("Receiver");
#endif
}

void loop()
{
  bool sbus;
  uint32_t t=micros();
  if(t-t0 < 1000) return;
  t0=t;

  sbus=scansbus(t);
  if(scanppm() || sbus) {
    send_report(); 
#ifdef USE_SERIALSTRING
    Serial.print(rpt_data.buttons,HEX); Serial.print(' ');
    Serial.print(rpt_data.X); Serial.print(' ');
    Serial.print(rpt_data.Y); Serial.print(' ');
    Serial.print(rpt_data.Rx); Serial.print(' ');
    Serial.print(rpt_data.Ry);
    Serial.println();
#endif
  }
}
