#include <Arduino.h>
#include <HID.h>
#include "usb_gpad.h"
// moving to https://github.com/arduino/Arduino/wiki/PluggableUSB-and-PluggableHID-howto


struct usb_gamepad_rpt rpt_data;

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
