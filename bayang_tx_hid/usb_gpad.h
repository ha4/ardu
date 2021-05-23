#ifndef __MY_USB_GAMEPAD_H_
#define __MY_USB_GAMEPAD_H_

struct usb_gamepad_rpt {
  uint8_t buttons;
  uint16_t leftX;
  uint16_t leftY;
  uint16_t rightX;
  uint16_t rightY;
};  // 9 bytes

extern struct usb_gamepad_rpt rpt_data;

void mygamepad_init();
void send_report();

#endif
