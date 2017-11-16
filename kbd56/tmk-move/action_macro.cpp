#include <Arduino.h>

#include "config.h"

#include "keyboard.h"
#include "keycode.h"

#include "report.h"

#include "action_code.h"
#include "action_macro.h"
#include "action.h"
#include "action_util.h"


#ifndef NO_ACTION_MACRO

#define MACRO_READ()  (macro = MACRO_GET(macro_p++))
void action_macro_play(const macro_t *macro_p)
{
    macro_t macro = END;
    uint8_t interval = 0;

    uint8_t mod_storage = 0;

    if (!macro_p) return;
    while (true) {
        switch (MACRO_READ()) {
            case KEY_DOWN:
                MACRO_READ();
                if (IS_MOD(macro)) {
                    add_weak_mods(MOD_BIT(macro));
                    send_keyboard_report();
                } else {
                    register_code(macro);
                }
                break;
            case KEY_UP:
                MACRO_READ();
                if (IS_MOD(macro)) {
                    del_weak_mods(MOD_BIT(macro));
                    send_keyboard_report();
                } else {
                    unregister_code(macro);
                }
                break;
            case WAIT:
                MACRO_READ();
                { uint8_t ms = macro; while (ms--) delay(1); }
                break;
            case INTERVAL:
                interval = MACRO_READ();
                break;
            case MOD_STORE:
                mod_storage = get_mods();
                break;
            case MOD_RESTORE:
                set_mods(mod_storage);
                send_keyboard_report();
                break;
            case MOD_CLEAR:
                clear_mods();
                send_keyboard_report();
                break;
            case 0x04 ... 0x73:
                register_code(macro);
                break;
            case 0x84 ... 0xF3:
                unregister_code(macro&0x7F);
                break;
            case END:
            default:
                return;
        }
        // interval
        { uint8_t ms = interval; while (ms--) delay(1); }
    }
}
#endif
