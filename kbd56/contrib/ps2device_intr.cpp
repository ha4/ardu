#include <Arduino.h>

#include "config.h"

#include "ps2device.h"


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




static void clkHi(void)
{
  PS2_CLK_DDR  &= ~_BV(PS2_CLK_PIN);
  PS2_CLK_PORT |= _BV(PS2_CLK_PIN);
}

static void clkLo(void)
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

#define clkPin() (PS2_CLK_IN & _BV(PS2_CLK_PIN))
#define dataPin() (PS2_DATA_IN & _BV(PS2_DATA_PIN))



static volatile unsigned char   ps2_rx;
static volatile unsigned char   ps2_tx;
static volatile unsigned char   ps2_shift;
static volatile unsigned char   ps2_parity;


volatile unsigned char  ps2_flags;

#define PS2_CLOCK_HIGH  0x01
#define PS2_RX_BAD      0x02
#define PS2_RX_BYTE     0x04
#define PS2_TX_BYTE     0x08
#define PS2_TX_OK       0x10
#define PS2_TX_ERR	0x20


volatile unsigned char  ps2_state;

enum enum_state {
  IDLE_START = 0, IDLE_WAIT_REL, 
  IDLE_OK_TO_TX, IDLE_END,

  RX_START = IDLE_END+10, RX_RELCLK, RX_DATA0,
  RX_DATA1, RX_DATA2, RX_DATA3, RX_DATA4,
  RX_DATA5, RX_DATA6, RX_DATA7,
  RX_PARITY, RX_STOP, RX_SENT_ACK, RX_END,

  TX_START = RX_END+10, 
  TX_DATA1, TX_DATA2, TX_DATA3, TX_DATA4,
  TX_DATA5, TX_DATA6, TX_DATA7,
  TX_PARITY, TX_STOP, TX_AFTER_STOP, TX_END
};

void ps2_init(void)
{
  ps2_state = IDLE_START;
  ps2_flags = PS2_CLOCK_HIGH | PS2_TX_OK;

  clkHi();
  dataHi();
}

void ps2_txSet(unsigned char d)
{
  ps2_tx = d;
  noInterrupts();
  ps2_flags &= ~PS2_TX_OK;
  ps2_flags |= PS2_TX_BYTE;
  interrupts();
}

unsigned char ps2_rxGet(void)
{
  noInterrupts();
  ps2_flags &= ~PS2_RX_BYTE;
  interrupts();
  return ps2_rx;
}

bool ps2_rxAvailable(void)
{
  return (ps2_flags & PS2_RX_BYTE);
}

bool ps2_txClear(void)
{
  return (ps2_flags & PS2_TX_OK);
}

/* call this routine every 33 us */
void ps2_process(void)
{
  if (ps2_state < IDLE_END) { /* start, wait_rel or ready to tx */
    dataHi();
    if (!(ps2_flags & PS2_CLOCK_HIGH)) /* clock low */
    {
      ps2_flags |= PS2_CLOCK_HIGH;
      clkHi();
      return;
    }
    /* if clock held low, then we must prepare to start rxing */
    if (!clkPin()) {
      ps2_state = IDLE_WAIT_REL;
      return;
    }
    switch(ps2_state) {
    case IDLE_START:
      ps2_state = IDLE_OK_TO_TX;
      return;
    case IDLE_OK_TO_TX:
      if (ps2_flags & PS2_TX_BYTE) {
        dataLo();
        ps2_state = TX_START;
      }
      return;
    case IDLE_WAIT_REL:
      if (!dataPin()) {
        /* PC wants to transmit */
        ps2_state = RX_START;
        return;
      }
      /* just an ack or something */
      ps2_state = IDLE_OK_TO_TX;
      return;
    }
    return;
  }
  else
    if (ps2_state < RX_END) {
      if (!(ps2_flags & PS2_CLOCK_HIGH)) {
        ps2_flags |= PS2_CLOCK_HIGH;
        clkHi();
        return;
      }
      /* at this point clock is high in preparation to going low */
      if (!clkPin()) /* PC is still holding clock down */
      { 
        dataHi();  
        ps2_state = IDLE_WAIT_REL; 
        return; 
      }

      switch(ps2_state) {
      case RX_START: /* PC has released clock line, we keep it high for a good half cycle */
        ps2_flags &= ~PS2_RX_BAD;
        ps2_state++;
        return;
      case RX_RELCLK: 
        break; /* now PC has seen clock high, show it some low */
      case RX_DATA0:
        ps2_flags &= ~PS2_RX_BYTE;
        if (dataPin()) { 
          ps2_rx = 0x80; 
          ps2_parity = 1; 
        } 
        else { 
          ps2_rx = 0; 
          ps2_parity = 0; 
        }
        break; /* end clk hi 1 */
      case RX_DATA1: 
      case RX_DATA2: 
      case RX_DATA3: 
      case RX_DATA4: 
      case RX_DATA5: 
      case RX_DATA6: 
      case RX_DATA7: 
        ps2_rx >>= 1;
        if (dataPin()) { 
          ps2_rx |= 0x80; 
          ps2_parity++; 
        }
        break; /* end clk hi 2 to 8 */
      case RX_PARITY: 
        if (dataPin()) ps2_parity++;
        if (!(ps2_parity & 1)) /* faulty, not odd parity */
          ps2_flags |= PS2_RX_BAD;
        break; /* end clk hi 9 */
      case RX_STOP: 
        if (!dataPin()) /* if stop bit not seen */
          ps2_flags |= PS2_RX_BAD;
        if (!(ps2_flags & PS2_RX_BAD)) { 
          dataLo();  
          ps2_flags |= PS2_RX_BYTE; 
        }
        break; /* end clk hi 10 */
      case RX_SENT_ACK: 
        dataHi();  
        ps2_state = IDLE_START; 
        return;
        /* remains in clk hi 11 */

      }
      clkLo(); 
      ps2_flags &= ~(PS2_CLOCK_HIGH);
      ps2_state++;
      return;
    }
    else
      if (ps2_state < TX_END) {
        if (ps2_flags & PS2_CLOCK_HIGH) {
          if (!clkPin()) { /* PC is still holding clock down */
            dataHi();
            ps2_state = IDLE_WAIT_REL;
            ps2_flags |= PS2_TX_ERR|PS2_TX_OK;
            return;
          }
          ps2_flags &= ~PS2_CLOCK_HIGH;
          clkLo();
          return;
        }
        /* at this point clock is low in preparation to going high */
        ps2_flags |= PS2_CLOCK_HIGH;
        clkHi();
        switch(ps2_state) {
        case TX_START:
          ps2_flags &= ~PS2_TX_ERR;  
          ps2_shift = ps2_tx; 
          ps2_parity = 0;
        case TX_DATA1: 
        case TX_DATA2: 
        case TX_DATA3: 
        case TX_DATA4:
        case TX_DATA5: 
        case TX_DATA6: 
        case TX_DATA7:
          if (ps2_shift & 1) { 
            dataHi(); 
            ps2_parity++; 
          }
          else dataLo();
          ps2_shift >>= 1;
          break; /* clock hi 1 to 8 */
        case TX_PARITY: 
          if (ps2_parity & 1) dataLo();
          else  dataHi();
          ps2_flags &= ~PS2_TX_BYTE;
          ps2_flags |= PS2_TX_OK;
          break; /* clock hi 9 */
        case TX_STOP: 
          dataHi();
          break; /* clock hi 10 */
        case TX_AFTER_STOP:
          ps2_state = IDLE_START;
          return;
          /* remains in clk hi 11 */
        }
        ps2_state++;
      }
}

