
#define MACRO_NONE      0
#define MACRO(...)      ({ static const macro_t __m[] PROGMEM = { __VA_ARGS__ }; &__m[0]; })
#define MACRO_GET(p)    pgm_read_byte(p)

typedef uint8_t macro_t;


#ifndef NO_ACTION_MACRO
void action_macro_play(const macro_t *macro_p);
#else
#define action_macro_play(macro)
#endif



/* Macro commands
 *   code(0x04-73)                      // key down(1byte)
 *   code(0x04-73) | 0x80               // key up(1byte)
 *   { KEY_DOWN, code(0x04-0xff) }      // key down(2bytes)
 *   { KEY_UP,   code(0x04-0xff) }      // key up(2bytes)
 *   WAIT                               // wait milli-seconds
 *   INTERVAL                           // set interval between macro commands
 *   END                                // stop macro execution
 *
 * Ideas(Not implemented):
 *   modifiers
 *   system usage
 *   consumer usage
 *   unicode usage
 *   function call
 *   conditionals
 *   loop
 */
enum macro_command_id{
    /* 0x00 - 0x03 */
    END                 = 0x00,
    KEY_DOWN,
    KEY_UP,

    /* 0x04 - 0x73 (reserved for keycode down) */

    /* 0x74 - 0x83 */
    WAIT                = 0x74,
    INTERVAL,
    MOD_STORE,
    MOD_RESTORE,
    MOD_CLEAR,

    /* 0x84 - 0xf3 (reserved for keycode up) */

    /* 0xf4 - 0xff */
};


/* TODO: keycode:0x04-0x73 can be handled by 1byte command  else 2bytes are needed
 * if keycode between 0x04 and 0x73
 *      keycode / (keycode|0x80)
 * else
 *      {KEY_DOWN, keycode} / {KEY_UP, keycode}
*/
#define DOWN(key)       KEY_DOWN, (key)
#define UP(key)         KEY_UP, (key)
#define TYPE(key)       DOWN(key), UP(key)
#define WAIT(ms)        WAIT, (ms)
#define INTERVAL(ms)    INTERVAL, (ms)
#define STORE()         MOD_STORE
#define RESTORE()       MOD_RESTORE
#define CLEAR()         MOD_CLEAR

/* key down */
#define D(key)          DOWN(KC_##key)
/* key up */
#define U(key)          UP(KC_##key)
/* key type */
#define T(key)          TYPE(KC_##key)
/* wait */
#define W(ms)           WAIT(ms)
/* interval */
#define I(ms)           INTERVAL(ms)
/* store modifier(s) */
#define SM()            STORE()
/* restore modifier(s) */
#define RM()            RESTORE()
/* clear modifier(s) */
#define CM()            CLEAR()

/* for backward comaptibility */
#define MD(key)         DOWN(KC_##key)
#define MU(key)         UP(KC_##key)

