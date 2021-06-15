#ifndef __MY_VISUALS_H_
#define __MY_VISUALS_H_

#define dFL_VUSB 0x0001
#define dFL_USBSER 0x0002

extern volatile uint8_t d_updating;
extern volatile uint8_t d_page;
extern volatile uint16_t d_flags;
extern volatile uint16_t v_keys;

void v_init();
void v_update();
void v_loop();
void menu_command(uint16_t k, uint16_t ch);


#endif
