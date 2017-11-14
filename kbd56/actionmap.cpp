#include <Arduino.h>
#include <stdint.h>

#include "config.h"

#include "keyboard.h"
#include "report.h"
#include "action_code.h"
#include "action_macro.h"
#include "action.h"

#include "actionmap.h"


/* Keymapping with 16bit action codes */
/*extern */const action_t actionmaps[][MATRIX_ROWS][MATRIX_COLS] = {};


/* Converts key to action */
action_t action_for_key(uint8_t layer, keypos_t key)
{
  action_t a;
  a.code = pgm_read_word(&actionmaps[(layer)][(key.row)][(key.col)]);
  return a;
}

/* Macro */
const macro_t *action_get_macro(keyrecord_t *record, uint8_t id, uint8_t opt)
{
    return MACRO_NONE;
}

/* Function */
void action_function(keyrecord_t *record, uint8_t id, uint8_t opt)
{
}

