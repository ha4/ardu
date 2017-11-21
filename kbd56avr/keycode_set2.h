
#define IS_ERROR(code)           (0)
#define IS_ANY(code)             (KC_F9         <= (code) && (code) <= 0xFE)
#define IS_KEY(code)             ((!IS_MOD(code)) && (!IS_FN(code)))
#define IS_MOD(code)             (MOD_INDEX(code)<8)

#define IS_SPECIAL(code)         (KC_FN0 <= (code) && (code) <= KC_FN7))
#define IS_EXTENDED(code)        (KC_WWW_SEARCH <= (code) && (code) <= KC_PGUP))
#define IS_EXTRA(code)           (KC_PSCREEN <= (code) && (code) <= KC_F7))
#define IS_SYSTEM(code)          (KC_PWR == (code) || KC_SLEP == (code) || KC_WAKE==(code))
#define IS_CONSUMER(code)        (0)
#define IS_FN(code)              (KC_FN0       <= (code) && (code) <= KC_FN7)
#define IS_MOUSEKEY(code)        (0)
#define IS_MOUSEKEY_MOVE(code)   (0)
#define IS_MOUSEKEY_BUTTON(code) (0)
#define IS_MOUSEKEY_WHEEL(code)  (0)
#define IS_MOUSEKEY_ACCEL(code)  (0)

/* MSB:RGUI RALT RSHIFT RCTRL   LGUI LALT LSHIFT LCTRL:LSB */
/*     A7   91   59     94      9F   11   12     14*/
#define MT(code,mod,next) (((code)==KC_##mod)?0:1+(next))
#define MOD_INDEX(c) MT(c,LCTL,MT(c,LSFT,MT(c,LALT,MT(c,LGUI,MT(c,RCTL,MT(c,RSFT,MT(c,RALT,MT(c,RGUI,1))))))))
//#define MOD_INDEX(code) ((code) & 0x07)
#define MOD_BIT(code)   (1<<MOD_INDEX(code))

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
    KC_LALT=0x11,        /* MOD */
    KC_LSHIFT=0x12,      /* MOD */
    KC_LCTRL=0x14,       /* MOD */
    KC_Q=0x15,
    KC_1=0x16,
    KC_Z=0x1A,
    KC_S=0x1B,
    KC_A=0x1C,
    KC_W=0x1D,
    KC_2=0x1E,
    KC_C=0x21,
    KC_X=0x22,
    KC_D=0x23,
    KC_E=0x24,
    KC_4=0x25,
    KC_3=0x26,
    KC_SPACE=0x29,
    KC_V=0x2A,
    KC_F=0x2B,
    KC_T=0x2C,
    KC_R=0x2D,
    KC_5=0x2E,
    KC_N=0x31,
    KC_B=0x32,
    KC_H=0x33,
    KC_G=0x34,
    KC_Y=0x35,
    KC_6=0x36,
    KC_M=0x3A,
    KC_J=0x3B,
    KC_U=0x3C,
    KC_7=0x3D,
    KC_8=0x3E,
    KC_COMMA=0x41, /* , < */
    KC_K=0x42,
    KC_I=0x43,
    KC_O=0x44,
    KC_0=0x45,
    KC_9=0x46,
    KC_DOT=0x49,   /* .  > */
    KC_SLASH=0x4A, /* /  ? */
    KC_L=0x4B,
    KC_SCOLON=0x4C,/* ; : */
    KC_P=0x4D,
    KC_MINUS=0x4E,
    KC_QUOTE=0x52, /* ' " */
    KC_LBRACKET=0x54,
    KC_EQUAL=0x55,
    KC_CAPSLOCK=0x58,
    KC_RSHIFT=0x59, /* MOD */
    KC_ENTER=0x5A,
    KC_RBRACKET=0x5B,
    KC_BSLASH=0x5D, /* \ | */
    KC_BSPACE=0x66,
    KC_KP_1=0x69,
    KC_KP_4=0x6B,
    KC_KP_7=0x6C,
    KC_KP_0=0x70,
    KC_KP_DOT=0x71,
    KC_KP_2=0x72,
    KC_KP_5=0x73,
    KC_KP_6=0x74,
    KC_KP_8=0x75,
    KC_ESCAPE=0x76,
    KC_NUMLOCK=0x77,
    KC_F11=0x78,
    KC_KP_PLUS=0x79,
    KC_KP_3=0x7A,
    KC_KP_MINUS=0x7B,
    KC_KP_ASTERISK=0x7C,
    KC_KP_9=0x7D,
    KC_SCROLLLOCK=0x7E,
};

enum ps2_set2_scancode_extend {
    KC_PSCREEN=0x81, /* E0 12 E0 7C */
    KC_PAUSE=0x82, /* E1 14 77 make+break, CTRL+PAUSE: E0 7E make+break */
    KC_F7=0x83, /* 83 */
    KC_WWW_SEARCH=0x90, /* E0 10 Media Control */
    KC_RALT=0x91, /* E0 11  MOD */
    KC_RCTRL=0x94, /* E0 14 MOD */
    KC_MEDIA_PREV_TRACK=0x95, /* E0 15 Media Control */
    KC_WWW_FAVORITES=0x98, /* E0 18 Media Control */
    KC_LGUI=0x9F, /* E0 1F MOD */
    KC_WWW_REFRESH=0xA0, /* E0 20 Media Control */
    KC_AUDIO_VOL_DOWN=0xA1, /* E0 21 Media Control */
    KC_AUDIO_MUTE=0xA3,  /* E0 23 Media Control */
    KC_RGUI=0xA7, /* E0 27 MOD */
    KC_WWW_STOP=0xA8, /* E0 28 Media Control */
    KC_CALCULATOR=0xAB, /* E0 2B Media Control */
    KC_APPLICATION=0xAF, /* E0 2F */
    KC_WWW_FORWARD=0xB0, /* E0 30 Media Control */
    KC_AUDIO_VOL_UP=0xB2, /* E0 32 Media Control */
    KC_MEDIA_PLAY_PAUSE=0xB4, /* E0 34 Media Control */
    KC_SYSTEM_POWER=0xB7, /* E0 37 System Control */
    KC_WWW_BACK=0xB8, /* E0 38 Media Control */
    KC_WWW_HOME=0xBA, /* E0 3A Media Control */
    KC_MEDIA_STOP=0xBB, /* E0 3B Media Control */
    KC_SYSTEM_SLEEP=0xBF, /* E0 3F System Control */
    KC_MY_COMPUTER=0xC0, /* E0 40 Media Control */
    KC_MAIL=0xC8, /* E0 48 Media Control */
    KC_KP_SLASH=0xCA, /* E0 4A */
    KC_MEDIA_NEXT_TRACK=0xCD, /* E0 4D Media Control */
    KC_MEDIA_SELECT=0xD0, /* E0 50 Media Control */
    KC_KP_ENTER=0xDA, /* E0 5A */
    KC_SYSTEM_WAKE=0xDE, /* E0 5E System Control */

    KC_END=0xE9, /* E0 69 */
    KC_LEFT=0xEB, /* E0 6B */
    KC_HOME=0xEC, /* E0 6C */
    KC_INSERT=0xF0, /* E0 70 */
    KC_DELETE=0xF1, /* E0 71 */
    KC_DOWN=0xF2, /* E0 72 */
    KC_RIGHT=0xF4, /* E0 74 */
    KC_UP=0xF5, /*  E0 75 */
    KC_PGDOWN=0xFA, /* E0 7A */ 
    KC_PGUP=0xFD, /* E0 7D */

    KC_FN0=0xE0,
    KC_FN1,
    KC_FN2,
    KC_FN3,
    KC_FN4,
    KC_FN5,
    KC_FN6,
    KC_FN7,

    KC_TRANSPARENT=0xFF,
};

