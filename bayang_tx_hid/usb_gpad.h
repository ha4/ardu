#ifndef __MY_USB_GAMEPAD_H_
#define __MY_USB_GAMEPAD_H_

struct usb_gamepad_rpt {
  int16_t leftX;
  int16_t leftY;
  int16_t rightX;
  int16_t rightY;
  uint8_t buttons;
};  // 9 bytes

extern struct usb_gamepad_rpt rpt_data;

void mygamepad_init();
void send_report();

#endif
