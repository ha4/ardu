
// convert to L string
#define LSTR(s) XLSTR(s)
#define XLSTR(s) L ## #s
// convert to string
#define STR(s) XSTR(s)
#define XSTR(s) #s


#ifdef __cplusplus
extern "C" {
#endif

uint8_t bitpop(uint8_t bits);
uint8_t bitpop16(uint16_t bits);
uint8_t bitpop32(uint32_t bits);

uint8_t biton(uint8_t bits);
uint8_t biton16(uint16_t bits);
uint8_t biton32(uint32_t bits);

uint8_t  bitrev(uint8_t bits);
uint16_t bitrev16(uint16_t bits);
uint32_t bitrev32(uint32_t bits);

#ifdef __cplusplus
}
#endif

