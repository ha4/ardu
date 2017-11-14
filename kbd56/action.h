
#ifdef __cplusplus
extern "C" {
#endif

/* tapping count and state */
typedef struct {
    bool    interrupted :1;
    bool    reserved2   :1;
    bool    reserved1   :1;
    bool    reserved0   :1;
    uint8_t count       :4;
} tap_t;

/* Key event container for recording */
typedef struct {
    keyevent_t  event;
#ifndef NO_ACTION_TAPPING
    tap_t tap;
#endif
} keyrecord_t;

/* Execute action per keyevent */
void action_exec(keyevent_t event);

/* action for key */
action_t action_for_key(uint8_t layer, keypos_t key);

/* macro */
const macro_t *action_get_macro(keyrecord_t *record, uint8_t id, uint8_t opt);

/* user defined special function */
void action_function(keyrecord_t *record, uint8_t id, uint8_t opt);

/* Utilities for actions.  */
void process_action(keyrecord_t *record);
void register_code(uint8_t code);
void unregister_code(uint8_t code);
void register_mods(uint8_t mods);
void unregister_mods(uint8_t mods);
//void set_mods(uint8_t mods);
void clear_keyboard(void);
void clear_keyboard_but_mods(void);
void layer_switch(uint8_t new_layer);
bool is_tap_key(keyevent_t event);

/* debug */
void debug_event(keyevent_t event);
void debug_record(keyrecord_t record);
void debug_action(action_t action);

#ifdef __cplusplus
}
#endif
