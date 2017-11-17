/* keymap definition macro
 * note: K3B equal K30, K42 equal K20
 */
#define KEYMAP( \
    K00, K01, K02, K03, K04, K05, K06, K07, K08, K09, K0A, K0B, K0C, K0D, K0E, \
    K10,  K11, K12, K13, K14, K15, K16, K17, K18, K19, K1A, K1B, K1C,   K1D,   \
    K20,   K21, K22, K23, K24, K25, K26, K27, K28, K29, K2A, K2B,     K2C,     \
    K30,     K31, K32, K33, K34, K35, K36, K37, K38, K39, K3A,       K3B,      \
              K40,                     K41,               K42, K43             \
) { \
/*0*/    { KC_##K00, KC_##K01, KC_##K02, KC_##K03, KC_##K04, KC_##K05, KC_##K06 }, \
/*1*/    { KC_##K10, KC_##K11, KC_##K12, KC_##K13, KC_##K14, KC_##K15, KC_##K16 }, \
/*2*/    { KC_##K20, KC_##K21, KC_##K22, KC_##K23, KC_##K24, KC_##K25, KC_##K26 }, \
/*3*/    { KC_##K30, KC_##K31, KC_##K32, KC_##K33, KC_##K34, KC_##K35, KC_##K36 }, \
/*4*/    { KC_##K37, KC_##K38, KC_##K39, KC_##K3A, KC_##K41, KC_##K43, KC_##K40 }, \
/*5*/    { KC_##K27, KC_##K28, KC_##K29, KC_##K2A, KC_##K2B, KC_##K1D, KC_##K0E }, \
/*6*/    { KC_##K17, KC_##K18, KC_##K19, KC_##K1A, KC_##K1B, KC_##K1C, KC_##K2C }, \
/*7*/    { KC_##K07, KC_##K08, KC_##K09, KC_##K0A, KC_##K0B, KC_##K0C, KC_##K0D }  \
}

