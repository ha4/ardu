#include <Arduino.h>
#include <stdbool.h>
#include "wait.h"
#include "ps2.h"
#include "ps2_io.h"
#include "debug.h"


#ifndef PS2_CLK_PORT
#define PS2_CLK_PORT    PORTB
#define PS2_CLK_DDR     DDRB
#define PS2_CLK_IN      PINB
#define PS2_CLK_PIN	0 /* D8 */
#endif

#ifndef PS2_DATA_PORT
#define PS2_DATA_PORT   PORTB
#define PS2_DATA_DDR    DDRB
#define PS2_DATA_IN     PINB
#define PS2_DATA_PIN    2 /* D10 */
#endif



static void clockHi(void)
{
  PS2_CLK_DDR  &= ~_BV(PS2_CLK_PIN);
  PS2_CLK_PORT |= _BV(PS2_CLK_PIN);
}

static void clockLo(void)
{
  PS2_CLK_PORT &= ~_BV(PS2_CLK_PIN);
  PS2_CLK_DDR  |= _BV(PS2_CLK_PIN);
}

static void dataHi(void)
{
  PS2_DATA_DDR  &= ~_BV(PS2_DATA_PIN);
  PS2_DATA_PORT |= _BV(PS2_DATA_PIN);
}

static void dataLo(void)
{
  PS2_DATA_PORT &= ~_BV(PS2_DATA_PIN);
  PS2_DATA_DDR  |= _BV(PS2_DATA_PIN);
}

#define clockPin() (PS2_CLK_IN & _BV(PS2_CLK_PIN))
#define dataPin() (PS2_DATA_IN & _BV(PS2_DATA_PIN))

#define WAIT(stat, us, err) do { \
    if (!wait_##stat(us)) { \
        ps2_error = err; \
        goto ERROR; \
    } \
} while (0)


/*--------------------------------------------------------------------
 * static functions
 *------------------------------------------------------------------*/
static inline uint16_t wait_clkLo(uint16_t us)
{
    while (clock_in()  && us) { asm(""); wait_us(1); us--; }
    return us;
}
static inline uint16_t wait_clkHi(uint16_t us)
{
    while (!clock_in() && us) { asm(""); wait_us(1); us--; }
    return us;
}
static inline uint16_t wait_dataLo(uint16_t us)
{
    while (data_in() && us)  { asm(""); wait_us(1); us--; }
    return us;
}
static inline uint16_t wait_dataHi(uint16_t us)
{
    while (!data_in() && us)  { asm(""); wait_us(1); us--; }
    return us;
}

/* idle state that device can send */
static inline void idle(void)
{
    clock_hi();
    data_hi();
}

/* inhibit device to send */
static inline void inhibit(void)
{
    clock_lo();
    data_hi();
}

uint8_t ps2_error = PS2_ERR_NONE;


void ps2_init(void)
{
    clock_init();
    data_init();

    // POR(150-2000ms) plus BAT(300-500ms) may take 2.5sec([3]p.20)
    wait_ms(2500);

    inhibit();
}

uint8_t ps2_send(uint8_t data)
{
    bool parity = true;
    ps2_error = PS2_ERR_NONE;

    /* terminate a transmission if we have */
    inhibit();
    wait_us(100); // 100us [4]p.13, [5]p.50

    /* 'Request to Send' and Start bit */
    data_lo();
    clock_hi();
    WAIT(clock_lo, 10000, 10);   // 10ms [5]p.50

    /* Data bit */
    for (uint8_t i = 0; i < 8; i++) {
        wait_us(15);
        if (data&(1<<i)) {
            parity = !parity;
            data_hi();
        } else {
            data_lo();
        }
        WAIT(clock_hi, 50, 2);
        WAIT(clock_lo, 50, 3);
    }

    /* Parity bit */
    wait_us(15);
    if (parity) { data_hi(); } else { data_lo(); }
    WAIT(clock_hi, 50, 4);
    WAIT(clock_lo, 50, 5);

    /* Stop bit */
    wait_us(15);
    data_hi();

    /* Ack */
    WAIT(data_lo, 50, 6);
    WAIT(clock_lo, 50, 7);

    /* wait for idle state */
    WAIT(clock_hi, 50, 8);
    WAIT(data_hi, 50, 9);

    inhibit();
    return ps2_host_recv_response();
ERROR:
    inhibit();
    return 0;
}

/* receive data when host want else inhibit communication */
uint8_t ps2_recv_response(void)
{
    // Command may take 25ms/20ms at most([5]p.46, [3]p.21)
    // 250 * 100us(wait for start bit in ps2_host_recv)
    uint8_t data = 0;
    uint8_t try = 250;
    do {
        data = ps2_host_recv();
    } while (try-- && ps2_error);
    return data;
}

/* called after start bit comes */
uint8_t ps2_host_recv(void)
{
    uint8_t data = 0;
    bool parity = true;
    ps2_error = PS2_ERR_NONE;

    /* release lines(idle state) */
    idle();

    /* start bit [1] */
    WAIT(clock_lo, 100, 1); // TODO: this is enough?
    WAIT(data_lo, 1, 2);
    WAIT(clock_hi, 50, 3);

    /* data [2-9] */
    for (uint8_t i = 0; i < 8; i++) {
        WAIT(clock_lo, 50, 4);
        if (data_in()) {
            parity = !parity;
            data |= (1<<i);
        }
        WAIT(clock_hi, 50, 5);
    }

    /* parity [10] */
    WAIT(clock_lo, 50, 6);
    if (data_in() != parity) {
        ps2_error = PS2_ERR_PARITY;
        goto ERROR;
    }
    WAIT(clock_hi, 50, 7);

    /* stop bit [11] */
    WAIT(clock_lo, 50, 8);
    WAIT(data_hi, 1, 9);
    WAIT(clock_hi, 50, 10);

    inhibit();
    return data;
ERROR:
    if (ps2_error > PS2_ERR_STARTBIT3) {
        xprintf("x%02X\n", ps2_error);
    }
    inhibit();
    return 0;
}

/* send LED state to keyboard */
void ps2_host_set_led(uint8_t led)
{
    ps2_host_send(0xED);
    ps2_host_send(led);
}
