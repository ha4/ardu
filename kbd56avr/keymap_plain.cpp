#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

#include "config.h"

#include "keycode_set2.h"
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
     *           | Alt |         Space             |Ctrl |FN|
     *           `------------------------------------------'
     */
    KEYMAP(ESC, 1,   2,   3,   4,   5,   6,   7,   8,   9,   0,MINS, EQL,BSLS, GRV, \
           TAB, Q,   W,   E,   R,   T,   Y,   U,   I,   O,   P,LBRC,RBRC,BSPC, \
           LCTL,A,   S,   D,   F,   G,   H,   J,   K,   L,SCLN,QUOT, ENT, \
           LSFT,Z,   X,   C,   V,   B,   N,   M,COMM, DOT, SLSH, RSFT, \
                LALT,                SPC,                  RCTL, FN0  ),
    /* Overlay 1: FN0
     * ,-----------------------------------------------------------.
     * |GUI| F1| F2| F3| F4| F5| F6| F7| F8| F9|F10|F11|F12|nLK|Ins|
     * |-----------------------------------------------------------|
     * |     |Hom|Up |End|PgU|   |   |Hom| P*|End|Psc|Slk|Pau|Del  |
     * |-----------------------------------------------------------|
     * |R-Ctrl|Lef|Dow|Rig|PgD|   |Lef|Dow|Up |Rig|   |   |        |
     * |-----------------------------------------------------------|
     * |R-Shift |   |   |   |   |PgU|   |   | P-| P+|App|R-Shift   |
     * `-----------------------------------------------------------'
     *           |RAlt|                            |RCtrl|  |
     *           `------------------------------------------'
     */
    KEYMAP(
        LGUI, F1,  F2,  F3,  F4,  F5,  F6,  F7,  F8,  F9,  F10, F11, F12,NLCK, INS, \
        TRNS,HOME, UP, END, PGUP,TRNS,TRNS,HOME,PAST, END,PSCR,SLCK,PAUS, DEL, \
        RCTL,LEFT,DOWN,RGHT,PGDN,TRNS,LEFT,DOWN, UP, RGHT,TRNS,TRNS,TRNS, \
        RSFT,TRNS,TRNS,TRNS,TRNS,PGUP,TRNS,TRNS,PMNS,PPLS, APP,TRNS, \
                  RALT,          TRNS,                    TRNS,TRNS),

};

uint8_t keymap_get_keycode(uint8_t layer, uint8_t row, uint8_t col)
{
  return pgm_read_byte(&keymaps[layer][row][col]);
}

