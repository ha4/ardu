
/*
 * Default Layer
 */
extern uint32_t default_layer_state;
void default_layer_debug(void);
void default_layer_set(uint32_t state);

#ifndef NO_ACTION_LAYER
/* bitwise operation */
void default_layer_or(uint32_t state);
void default_layer_and(uint32_t state);
void default_layer_xor(uint32_t state);
#endif


/*
 * Keymap Layer
 */
#ifndef NO_ACTION_LAYER
extern uint32_t layer_state;
void layer_debug(void);
void layer_clear(void);
void layer_move(uint8_t layer);
void layer_on(uint8_t layer);
void layer_off(uint8_t layer);
void layer_invert(uint8_t layer);
/* bitwise operation */
void layer_or(uint32_t state);
void layer_and(uint32_t state);
void layer_xor(uint32_t state);
#endif


/* return action depending on current layer status */
action_t layer_switch_get_action(keyevent_t key);
