
#define IS_ERROR(code)           (0)
#define IS_ANY(code)             (KC_F9         <= (code) && (code) <= 0xFE)
#define IS_KEY(code)             ((!IS_MOD(code)) && (!IS_FN(code)))
#define IS_MOD(code)             (KC_LCTRL == (code) || KC_RGUI == (code) || KC_LALT == (code) || KC_LSHIFT == (code) || \
                                  KC_RCTRL == (code) || KC_LGUI == (code) || KC_RALT == (code) || KC_RSHIFT == (code) )

#define IS_SPECIAL(code)         (KC_FN0 <= (code) && (code) <= KC_FN7))
#define IS_SYSTEM(code)          (KC_PWR == (code) || KC_SLEP == (code) || KC_WAKE==(code))
#define IS_CONSUMER(code)        (0)
#define IS_FN(code)              (KC_FN0       <= (code) && (code) <= KC_FN7)
#define IS_MOUSEKEY(code)        (0)
#define IS_MOUSEKEY_MOVE(code)   (0)
#define IS_MOUSEKEY_BUTTON(code) (0)
#define IS_MOUSEKEY_WHEEL(code)  (0)
#define IS_MOUSEKEY_ACCEL(code)  (0)

#define MOD_BIT(code)   (1<<MOD_INDEX(code))
#define MOD_INDEX(code) ((code) & 0x07)

#define FN_BIT(code)    (1<<FN_INDEX(code))
#define FN_INDEX(code)  ((code) - KC_FN0)
#define FN_MIN          KC_FN0
#define FN_MAX          KC_FN7


/*
 * Short names for ease of definition of keymap
 */
#define KC_LCTL KC_LCTRL
#define KC_RCTL KC_RCTRL
#define KC_LSFT KC_LSHIFT
#define KC_RSFT KC_RSHIFT
#define KC_ESC  KC_ESCAPE
#define KC_BSPC KC_BSPACE
#define KC_ENT  KC_ENTER
#define KC_DEL  KC_DELETE
#define KC_INS  KC_INSERT
#define KC_CAPS KC_CAPSLOCK
#define KC_CLCK KC_CAPSLOCK
#define KC_RGHT KC_RIGHT
#define KC_PGDN KC_PGDOWN
#define KC_PSCR KC_PSCREEN
#define KC_SLCK KC_SCROLLLOCK
#define KC_PAUS KC_PAUSE
#define KC_BRK  KC_PAUSE
#define KC_NLCK KC_NUMLOCK
#define KC_SPC  KC_SPACE
#define KC_MINS KC_MINUS
#define KC_EQL  KC_EQUAL
#define KC_GRV  KC_GRAVE
#define KC_RBRC KC_RBRACKET
#define KC_LBRC KC_LBRACKET
#define KC_COMM KC_COMMA
#define KC_BSLS KC_BSLASH
#define KC_SLSH KC_SLASH
#define KC_SCLN KC_SCOLON
#define KC_QUOT KC_QUOTE
#define KC_APP  KC_APPLICATION
#define KC_NUHS KC_NONUS_HASH
#define KC_NUBS KC_NONUS_BSLASH
#define KC_LCAP KC_LOCKING_CAPS
#define KC_LNUM KC_LOCKING_NUM
#define KC_LSCR KC_LOCKING_SCROLL
#define KC_ERAS KC_ALT_ERASE
#define KC_CLR  KC_CLEAR
/* Japanese specific */
#define KC_ZKHK KC_GRAVE
#define KC_RO   KC_INT1
#define KC_KANA KC_INT2
#define KC_JYEN KC_INT3
#define KC_JPY  KC_INT3
#define KC_HENK KC_INT4
#define KC_MHEN KC_INT5
/* Korean specific */
#define KC_HAEN KC_LANG1
#define KC_HANJ KC_LANG2
/* Keypad */
#define KC_P1   KC_KP_1
#define KC_P2   KC_KP_2
#define KC_P3   KC_KP_3
#define KC_P4   KC_KP_4
#define KC_P5   KC_KP_5
#define KC_P6   KC_KP_6
#define KC_P7   KC_KP_7
#define KC_P8   KC_KP_8
#define KC_P9   KC_KP_9
#define KC_P0   KC_KP_0
#define KC_PDOT KC_KP_DOT
#define KC_PCMM KC_KP_COMMA
#define KC_PSLS KC_KP_SLASH
#define KC_PAST KC_KP_ASTERISK
#define KC_PMNS KC_KP_MINUS
#define KC_PPLS KC_KP_PLUS
#define KC_PEQL KC_KP_EQUAL
#define KC_PENT KC_KP_ENTER
/* Unix function key */
#define KC_EXEC KC_EXECUTE
#define KC_SLCT KC_SELECT
#define KC_AGIN KC_AGAIN
#define KC_PSTE KC_PASTE
/* Mousekey */
#define KC_MS_U KC_MS_UP
#define KC_MS_D KC_MS_DOWN
#define KC_MS_L KC_MS_LEFT
#define KC_MS_R KC_MS_RIGHT
#define KC_BTN1 KC_MS_BTN1
#define KC_BTN2 KC_MS_BTN2
#define KC_BTN3 KC_MS_BTN3
#define KC_BTN4 KC_MS_BTN4
#define KC_BTN5 KC_MS_BTN5
#define KC_WH_U KC_MS_WH_UP
#define KC_WH_D KC_MS_WH_DOWN
#define KC_WH_L KC_MS_WH_LEFT
#define KC_WH_R KC_MS_WH_RIGHT
#define KC_ACL0 KC_MS_ACCEL0
#define KC_ACL1 KC_MS_ACCEL1
#define KC_ACL2 KC_MS_ACCEL2
/* Sytem Control */
#define KC_PWR  KC_SYSTEM_POWER
#define KC_SLEP KC_SYSTEM_SLEEP
#define KC_WAKE KC_SYSTEM_WAKE
/* Consumer Page */
#define KC_MUTE KC_AUDIO_MUTE
#define KC_VOLU KC_AUDIO_VOL_UP
#define KC_VOLD KC_AUDIO_VOL_DOWN
#define KC_MNXT KC_MEDIA_NEXT_TRACK
#define KC_MPRV KC_MEDIA_PREV_TRACK
#define KC_MFFD KC_MEDIA_FAST_FORWARD
#define KC_MRWD KC_MEDIA_REWIND
#define KC_MSTP KC_MEDIA_STOP
#define KC_MPLY KC_MEDIA_PLAY_PAUSE
#define KC_EJCT KC_MEDIA_EJECT
#define KC_MSEL KC_MEDIA_SELECT
#define KC_MAIL KC_MAIL
#define KC_CALC KC_CALCULATOR
#define KC_MYCM KC_MY_COMPUTER
#define KC_WSCH KC_WWW_SEARCH
#define KC_WHOM KC_WWW_HOME
#define KC_WBAK KC_WWW_BACK
#define KC_WFWD KC_WWW_FORWARD
#define KC_WSTP KC_WWW_STOP
#define KC_WREF KC_WWW_REFRESH
#define KC_WFAV KC_WWW_FAVORITES
/* Jump to bootloader */
#define KC_BTLD KC_BOOTLOADER
/* Transparent */
#define KC_TRNS KC_TRANSPARENT



/* PS2 Set2 Keyboard Scan codes */
enum ps2_set2_keyboard_scancode {
    KC_NO               = 0x00,

    KC_F9=0x01,

    KC_F5=0x03,
    KC_F3=0x04,
    KC_F1=0x05,
    KC_F2=0x06,
    KC_F12=0x07,

    KC_F10=0x09,
    KC_F8=0x0A,
    KC_F6=0x0B,
    KC_F4=0x0C,
    KC_TAB=0x0D,
    KC_GRAVE=0x0E, /* ` ~ */

    KC_WWW_SEARCH=0x80, /* E0 10 Media Control */

    KC_LALT=0x11,
    KC_RALT=0x81, /* E0 11 */

    KC_LSHIFT=0x12,

    KC_LCTRL=0x14,
    KC_RCTRL=0x82, /* E0 14 */

    KC_Q=0x15,
    KC_MEDIA_PREV_TRACK=0x84, /* E0 15 Media Control */

    KC_1=0x16,

    KC_WWW_FAVORITES=0x85, /* E0 18 Media Control */

    KC_Z=0x1A,
    KC_S=0x1B,
    KC_A=0x1C,
    KC_W=0x1D,
    KC_2=0x1E,
    KC_LGUI=0x86, /* E0 1F */,
    KC_WWW_REFRESH=0x87, /* E0 20 Media Control */

    KC_C=0x21,
    KC_AUDIO_VOL_DOWN=0x88, /* E0 21 Media Control */

    KC_X=0x22,

    KC_D=0x23,
    KC_AUDIO_MUTE=0x89,  /* E0 23 Media Control */

    KC_E=0x24,
    KC_4=0x25,
    KC_3=0x26,
    KC_RGUI=0x8A, /* E0 27 */
    KC_WWW_STOP=0x8B, /* E0 28 Media Control */
    KC_SPACE=0x29,
    KC_V=0x2A,

    KC_F=0x2B,
    KC_CALCULATOR=0x8C, /* E0 2B Media Control */

    KC_T=0x2C,
    KC_R=0x2D,

    KC_5=0x2E,
    KC_APPLICATION=0x8D, /* E0 2F */

    KC_WWW_FORWARD=0x8E, /* E0 30 Media Control */
    KC_N=0x31,

    KC_B=0x32,
    KC_AUDIO_VOL_UP=0x8F, /* E0 32 Media Control */

    KC_H=0x33,

    KC_G=0x34,
    KC_MEDIA_PLAY_PAUSE=0x90, /* E0 34 Media Control */

    KC_Y=0x35,
    KC_6=0x36,
    KC_SYSTEM_POWER=0x91, /* E0 37 System Control */
    KC_WWW_BACK=0x92, /* E0 38 Media Control */

    KC_M=0x3A,
    KC_WWW_HOME=0x93, /* E0 3A Media Control */

    KC_J=0x3B,
    KC_MEDIA_STOP=0x94, /* E0 3B Media Control */

    KC_U=0x3C,
    KC_7=0x3D,
    KC_8=0x3E,
    KC_SYSTEM_SLEEP=0x95, /* E0 3F System Control */
    KC_MY_COMPUTER=0x96, /* E0 40 Media Control */
    KC_COMMA=0x41, /* , < */
    KC_K=0x42,
    KC_I=0x43,
    KC_O=0x44,
    KC_0=0x45,
    KC_9=0x46,

    KC_MAIL=0x97, /* E0 48 Media Control */
    KC_DOT=0x49,   /* .  > */
 
    KC_SLASH=0x4A, /* /  ? */
    KC_KP_SLASH=0x98, /* E0 4A */

    KC_L=0x4B,
    KC_SCOLON=0x4C,/* ; : */

    KC_P=0x4D,
    KC_MEDIA_NEXT_TRACK=0x99, /* E0 4D Media Control */

    KC_MINUS=0x4E,

    KC_MEDIA_SELECT=0x9A, /* E0 50 Media Control */

    KC_QUOTE=0x52, /* ' " */

    KC_LBRACKET=0x54,
    KC_EQUAL=0x55,


    KC_CAPSLOCK=0x58,
    KC_RSHIFT=0x59,

    KC_ENTER=0x5A,
    KC_KP_ENTER=0x9B, /* E0 5A */

    KC_RBRACKET=0x5B,

    KC_BSLASH=0x5D, /* \ | */
    KC_SYSTEM_WAKE=0x9C, /* E0 5E System Control */

    KC_BSPACE=0x66,

    KC_KP_1=0x69,
    KC_END=0x9D, /* E0 69 */

    KC_KP_4=0x6B,
    KC_LEFT=0x9E, /* E0 6B */

    KC_KP_7=0x6C,
    KC_HOME=0x9F, /* E0 6C */

    KC_KP_0=0x70,
    KC_INSERT=0xA0, /* E0 70 */

    KC_KP_DOT=0x71,
    KC_DELETE=0xA1, /* E0 71 */

    KC_KP_2=0x72,
    KC_DOWN=0xA2, /* E0 72 */

    KC_KP_5=0x73,

    KC_KP_6=0x74,
    KC_RIGHT=0xA3, /* E0 74 */

    KC_KP_8=0x75,
    KC_UP=0xA4, /*  E0 75 */

    KC_ESCAPE=0x76,
    KC_NUMLOCK=0x77,

    KC_F11=0x78,
    KC_KP_PLUS=0x79,

    KC_KP_3=0x7A,
    KC_PGDOWN=0xA5, /* E0 7A */ 

    KC_KP_MINUS=0x7B,
    KC_KP_ASTERISK=0x7C,

    KC_KP_9=0x7D,
    KC_PGUP=0xA6, /* E0 7D */

    KC_SCROLLLOCK=0x7E,

    KC_F7=0x83,

    KC_PSCREEN=0xA7, /* make */
    KC_PAUSE=0xC0, /* make+brake */
    
    KC_FN0=0xF0,
    KC_FN1,
    KC_FN2,
    KC_FN3,
    KC_FN4,
    KC_FN5,
    KC_FN6,
    KC_FN7,

    KC_TRANSPARENT=0xFF,
};

enum ps2_set2_scancode_extend {
	KE80_WWW_SEARCH=0x10E0, /* Media Control */
	KE81_RALT=0x11E0,
	KE82_RCTRL=0x14E0,
	KE83_F7=0x83, 
	KE84_MEDIA_PREV_TRACK=0x15E0, /* Media Control */
	KE85_WWW_FAVORITES=0x18E0, /* Media Control */
	KE86_LGUI=0x1FE0,
	KE87_WWW_REFRESH=0x20E0, /* Media Control */
	KE88_AUDIO_VOL_DOWN=0x21E0, /* Media Control */
	KE89_AUDIO_MUTE=0x23E0,  /* Media Control */
	KE8A_RGUI=0x27E0,
	KE8B_WWW_STOP=0x28E0, /* Media Control */
	KE8C_CALCULATOR=0x2BE0, /* Media Control */
	KE8D_APPLICATION=0x2FE0,
	KE8E_WWW_FORWARD=0x30E0, /* Media Control */
	KE8F_AUDIO_VOL_UP=0x32E0, /* Media Control */
	KE90_MEDIA_PLAY_PAUSE=0x34E0, /* Media Control */
	KE91_SYSTEM_POWER=0x37E0, /* System Control */
	KE92_WWW_BACK=0x38E0, /* Media Control */
	KE93_WWW_HOME=0x3AE0, /* Media Control */
	KE94_MEDIA_STOP=0x3BE0, /* Media Control */
	KE95_SYSTEM_SLEEP=0x3FE0, /* System Control */
	KE96_MY_COMPUTER=0x40E0, /* Media Control */
	KE97_MAIL=0x48E0, /* Media Control */
	KE98_KP_SLASH=0x4AE0,
	KE99_MEDIA_NEXT_TRACK=0x4DE0, /* Media Control */
	KE9A_MEDIA_SELECT=0x50E0, /* Media Control */
	KE9B_KP_ENTER=0x5AE0,
	KE9C_SYSTEM_WAKE=0x5EE0,  /* System Control */
	KE9D_END=0x69E0,
	KE9E_LEFT=0x6BE0,
	KE9F_HOME=0x6CE0,
	KEA0_INSERT=0x70E0,
	KEA1_DELETE=0x71E0,
	KEA2_DOWN=0x72E0,
	KEA3_RIGHT=0x74E0,
	KEA4_UP=0x75E0,
	KEA5_PGDOWN=0x7AE0,
	KEA6_PGUP=0x7DE0,
	KEA7_PSCREEN=0x7CE012E0, /* make */
	KEC0_PAUSE=0x7714E1, /* make+brake macro */
};

