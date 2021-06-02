#include <Arduino.h>
#include <HID.h>
#include "usb_gpad.h"
// moving to https://github.com/arduino/Arduino/wiki/PluggableUSB-and-PluggableHID-howto

/* HID Class Report Descriptor */
/* Short items: size is 0, 1, 2 or 3 specifying 0, 1, 2 or 4 (four) bytes */
/* of data as per HID Class standard */
 
/* Main items */
#define INPUT(size)             (0x80 | size)
#define OUTPUT(size)            (0x90 | size)
#define FEATURE(size)           (0xb0 | size)
#define COLLECTION(size)        (0xa0 | size)
#define END_COLLECTION(size)    (0xc0 | size)
 
/* Global items */
#define USAGE_PAGE(size)        (0x04 | size)
#define LOGICAL_MINIMUM(size)   (0x14 | size)
#define LOGICAL_MAXIMUM(size)   (0x24 | size)
#define PHYSICAL_MINIMUM(size)  (0x34 | size)
#define PHYSICAL_MAXIMUM(size)  (0x44 | size)
#define UNIT_EXPONENT(size)     (0x54 | size)
#define UNIT(size)              (0x64 | size)
#define REPORT_SIZE(size)       (0x74 | size)
#define REPORT_ID(size)         (0x84 | size)
#define REPORT_COUNT(size)      (0x94 | size)
#define PUSH(size)              (0xa4 | size)
#define POP(size)               (0xb4 | size)
 
/* Local items */
#define USAGE(size)                 (0x08 | size)
#define USAGE_MINIMUM(size)         (0x18 | size)
#define USAGE_MAXIMUM(size)         (0x28 | size)
#define DESIGNATOR_INDEX(size)      (0x38 | size)
#define DESIGNATOR_MINIMUM(size)    (0x48 | size)
#define DESIGNATOR_MAXIMUM(size)    (0x58 | size)
#define STRING_INDEX(size)          (0x78 | size)
#define STRING_MINIMUM(size)        (0x88 | size)
#define STRING_MAXIMUM(size)        (0x98 | size)
#define DELIMITER(size)             (0xa8 | size)

/* HID PAGES */
#define HID_GENERIC_DESKTOP  0x01
#define HID_BUTTON  0x09

/* HID USAGES */
#define HID_GAME_PAD 0x05
#define HID_X 0x30
#define HID_Y 0x31
#define HID_Z 0x32
#define HID_RX 0x33
#define HID_RY 0x34
#define HID_RZ 0x35

/* COLLECTIONS */
#define C_PHYSICAL 0x00
#define C_APPLICATION 0x01
#define C_LOGICAL 0x02
#define C_VENDOR_DEFINED 0x02

/* INPUT/OUTPUT/FREATURE */
#define T_DATA  0x00
#define T_CONST 0x01
#define T_ARRAY 0x00
#define T_VARIABLE  0x02
#define T_ABSOLUTE 0x00
#define T_RELATIVE 0x04
#define T_NOWRAP   0x00
#define T_WRAP     0x08
#define T_LINEAR   0x00
#define T_NLIN     0x10
#define T_PREFSTATE 0x00
#define T_NPRF      0x20
#define T_NONULL    0x00
#define T_NULLSTATE 0x40
#define T_NONVOLATILE 0x00
#define T_VOLATILE  0x80
#define T_BITFIELD  0x00
#define T_BUFFERED  0x00, 0x01

struct usb_gamepad_rpt rpt_data;

const uint8_t _hid_rpt_descr[] PROGMEM = {
  USAGE_PAGE(1), HID_GENERIC_DESKTOP,
  USAGE(1), HID_GAME_PAD,
  COLLECTION(1), C_APPLICATION,
    COLLECTION(1), C_PHYSICAL,
    REPORT_ID(1), 0x03,

    USAGE_PAGE(1), HID_BUTTON,
    USAGE_MINIMUM(1), 0x01, //  (Button 1)
    USAGE_MAXIMUM(1), 0x08, //  (Button 8)
    LOGICAL_MINIMUM(1), 0,
    LOGICAL_MAXIMUM(1), 1,
    REPORT_SIZE(1), 1,
    REPORT_COUNT(1), 8,
    INPUT(1), T_DATA|T_VARIABLE|T_ABSOLUTE,

    USAGE_PAGE(1), HID_GENERIC_DESKTOP,
    USAGE(1), HID_X,
    USAGE(1), HID_Y,
    USAGE(1), HID_RX,
    USAGE(1), HID_RY,
//    PHYSICAL_MINIMUM(2), 0x00, 0x00, // 0
//    PHYSICAL_MAXIMUM(2), 0xff, 0x03, // 1023
    LOGICAL_MINIMUM(2), 0x00, 0x00,  // 0
    LOGICAL_MAXIMUM(2), 0xff, 0x03,  // 1023
    REPORT_SIZE(1), 16,
    REPORT_COUNT(1), 4,
    INPUT(1), T_DATA|T_VARIABLE|T_ABSOLUTE,

    END_COLLECTION(0),
  END_COLLECTION(0)
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

void mygamepad_init()
{
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
