#define PS2_CLK_PORT    PORTB
#define PS2_CLK_DDR     DDRB
#define PS2_CLK_IN      PINB
#define PS2_CLK_PIN	0 /* D8 */

#define PS2_DATA_PORT   PORTB
#define PS2_DATA_DDR    DDRB
#define PS2_DATA_IN     PINB
#define PS2_DATA_PIN    2 /* D10 */

/* key matrix size */
#define MATRIX_ROWS 8
#define MATRIX_COLS 7

/* define if matrix has ghost */
//#define MATRIX_HAS_GHOST

/* Set 0 if debouncing isn't needed */
#define DEBOUNCE    5


/* disable debug print */
#define NO_DEBUG

/* disable print */
#define NO_PRINT

#define NO_ACTION_LAYER
#define NO_ACTION_TAPPING
#define NO_ACTION_ONESHOT
#define NO_ACTION_MACRO
#define NO_ACTION_FUNCTION

