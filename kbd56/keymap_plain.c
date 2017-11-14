#include <stdint.h>
#include <stdbool.h>
#include "keycode.h"
#include "keymap.h"

#include "keymap_common.h"

// Caps Gui RCTL, RALT RSFT
const uint8_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /* Keymap 0: Default Layer
     * ,-----------------------------------------------------------.
     * |Esc|  1|  2|  3|  4|  5|  6|  7|  8|  9|  0|  -|  =|  \|  `|
     * |-----------------------------------------------------------|
     * |Tab  |  Q|  W|  E|  R|  T|  Y|  U|  I|  O|  P|  [|  ]|BckSp|
     * |-----------------------------------------------------------|
     * |Ctrl  |  A|  S|  D|  F|  G|  H|  J|  K|  L|  ;|  '|Return  |
     * |-----------------------------------------------------------|
     * |Shift   |  Z|  X|  C|  V|  B|  N|  M|  ,|  .|  /|Shift     |
     * `-----------------------------------------------------------'
     *           | Alt |         Space             |Ctrl|FN|
     *           `-----------------------------------------'
     */
    KEYMAP(ESC, 1,   2,   3,   4,   5,   6,   7,   8,   9,   0,MINS, EQL,BSLS, GRV, \
           TAB, Q,   W,   E,   R,   T,   Y,   U,   I,   O,   P,LBRC,RBRC,BSPC, \
           LCTL,A,   S,   D,   F,   G,   H,   J,   K,   L,SCLN,QUOT, ENT, \
           LSFT,Z,   X,   C,   V,   B,   N,   M,COMM, DOT, SLSH, RSFT, \
                LALT,                SPC,                     NO, FN0  ),
    /* Overlay 1: FN0
     * ,-----------------------------------------------------------.
     * |`  | F1| F2| F3| F4| F5| F6| F7| F8| F9|F10|F11|F12|   |Ins|
     * |-----------------------------------------------------------|
     * |     |   |   |   |   |   |   |Hom|Up |End|Psc|Slk|Pau|Del  |
     * |-----------------------------------------------------------|
     * |R-Ctrl|   |   |   |   |   |PgU|Lef|Dow|Rig|Up |   |        |
     * |-----------------------------------------------------------|
     * |R-Shift |   |   |   |   |Spc|PgD|   |   |   |   |L-Shift   |
     * `-----------------------------------------------------------'
     *           |RAlt|                          |RCtrl|   |
     *           `-----------------------------------------'
     */
    KEYMAP(
        GUI,  F1,  F2,  F3,  F4,  F5,  F6,  F7,  F8,  F9,  F10, F11, F12,TRNS, INS, \
        TRNS,TRNS,TRNS,TRNS,TRNS,TRNS,TRNS,HOME, UP, END, PSCR,SLCK,PAUS, DEL, \
        RCTL,TRNS,TRNS,TRNS,TRNS,TRNS,PGUP,LEFT,DOWN,RGHT, UP, TRNS,TRNS, \
        RSFT,TRNS,TRNS,TRNS,TRNS,SPC, PGDN,TRNS,TRNS,TRNS,TRNS,LSFT, \
                  RALT,          TRNS,                    TRNS,TRNS),

};

// correct it
//const action_t PROGMEM fn_actions[] = {
//    [0] = ACTION_LAYER_TAP_KEY(1, KC_SPACE),
//    [1] = ACTION_MODS_KEY(MOD_LSFT, KC_GRV),    // tilde
//};