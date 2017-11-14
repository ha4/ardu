/*
*/
#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "keyboard.h"
#include "matrix.h"
//#include "keymap.h"

//#include "host.h"
//#include "led.h"
//#include "keycode.h"
//#include "timer.h"
//#include "print.h"
//#include "debug.h"
//#include "command.h"
//#include "util.h"
//#include "sendchar.h"
//#include "bootmagic.h"
//#include "eeconfig.h"
//#include "backlight.h"
//#include "hook.h"

#include "action_code.h"
#include "action_macro.h"
#include "action.h"

void keyboard_setup(void)
{
    matrix_setup();
}

void keyboard_init(void)
{
//    timer_init();
    matrix_init();
}

/*
 * Do keyboard routine jobs: scan mantrix, light LEDs, ...
 * This is repeatedly called as fast as possible.
 */
void keyboard_task(void)
{
    static matrix_row_t matrix_prev[MATRIX_ROWS];
    static uint8_t led_status = 0;
    matrix_row_t matrix_row = 0;
    matrix_row_t matrix_change = 0;

    matrix_scan();
    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
        matrix_row = matrix_get_row(r);
        matrix_change = matrix_row ^ matrix_prev[r];
        if (matrix_change) {
            for (uint8_t c = 0; c < MATRIX_COLS; c++) {
                if (matrix_change & ((matrix_row_t)1<<c)) {
                    keyevent_t e = { (keypos_t){ r, c }, (matrix_row & ((matrix_row_t)1<<c)),
                        (millis() | 1) }; /* time should not be 0 */
                    action_exec(e);
                    // record a processed key
                    matrix_prev[r] ^= ((matrix_row_t)1<<c);
                }
            }
        }
    }
    // call with pseudo tick event when no real key event.
    action_exec(TICK);

}

void keyboard_set_leds(uint8_t leds)
{
}
