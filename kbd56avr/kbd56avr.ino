#include <stdint.h> 
#include <stdbool.h>
#include "config.h"
#include "keycode_set2.h"
#include "matrix.h"

#define FLA_CLOCK_HIGH	1
#define FLA_RX_BAD	2
#define FLA_RX_BYTE	4
#define FLA_TX_BYTE	8
#define FLA_TX_OK	0x10

#define FLA_TX_ERR	0x20

volatile unsigned char	kbd_flags;

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

volatile unsigned char kbd_state;

static volatile unsigned char	rx_byte;
static volatile unsigned char	tx_byte;
static volatile unsigned char	tx_shift;
static volatile unsigned char	parity;

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

/* FOSC 8000000, 8 cycles per us, prescaler=8, 33us  */
#define COUNT_UP    (256 - 33)

void timer2_init()
{
  TCCR2A=0;
  TCCR2B=(0b010<<CS20);// clk/div8, mode0
  TCNT2=COUNT_UP;  /* value counts up from this to zero */
  TIMSK2 |= _BV(TOIE2);  // enable TCNT2 overflow interrupt
}

ISR(TIMER2_OVF_vect)
{
  TCNT2 = COUNT_UP;
  if (kbd_state < IDLE_END) { // start, wait_rel or ready to tx
    dataHi();
    if (!(kbd_flags & FLA_CLOCK_HIGH)) {
      kbd_flags |= FLA_CLOCK_HIGH;
      clkHi();
      return;
    }
    /* if clock held low, then we must prepare to start rxing */
    if (!clkPin()) {
      kbd_state = IDLE_WAIT_REL;
      return;
    }

    switch(kbd_state) {
    case IDLE_START:
      kbd_state = IDLE_OK_TO_TX;
      return;
    case IDLE_WAIT_REL:
      if (!dataPin()) {
        /* PC wants to transmit */
        kbd_state = RX_START;
        return;
      }
      /* just an ack or something */
      kbd_state = IDLE_OK_TO_TX;
      return;
    case IDLE_OK_TO_TX:
      if (kbd_flags & FLA_TX_BYTE) {
        dataLo();
        kbd_state = TX_START;
      }
      return;
    }
    return;
  }
  else // end < IDLE_END
  if (kbd_state < RX_END) {
    if (!(kbd_flags & FLA_CLOCK_HIGH)) {
      kbd_flags |= FLA_CLOCK_HIGH;
      clkHi();
      return;
    }
    /* at this point clock is high in preparation to going low */
    if (!clkPin()) {
      /* PC is still holding clock down */
      dataHi();
      kbd_state = IDLE_WAIT_REL;
      return;
    }

    switch(kbd_state) {
    case RX_START:
      /* PC has released clock line */
      /* we keep it high for a good half cycle */
      kbd_flags &= ~FLA_RX_BAD;
      kbd_state++;
      return;
    case RX_RELCLK:
      /* now PC has seen clock high, show it some low */
      break;
    case RX_DATA0:
      kbd_flags &= ~FLA_RX_BYTE;
      if (dataPin()) {
        rx_byte = 0x80;
        parity = 1;
      } 
      else {
        parity = 0;
        rx_byte = 0;
      }
      break; /* end clk hi 1 */
    case RX_DATA1:	
    case RX_DATA2:	
    case RX_DATA3:	
    case RX_DATA4: 
    case RX_DATA5:	
    case RX_DATA6:	
    case RX_DATA7: 
      rx_byte >>= 1;
      if (dataPin()) {
        rx_byte |= 0x80;
        parity++;
      }
      break; /* end clk hi 2 to 8 */
    case RX_PARITY: 
      if (dataPin()) parity++;
      if (!(parity & 1)) /* faulty, not odd parity */
        kbd_flags |= FLA_RX_BAD;
      break; /* end clk hi 9 */
    case RX_STOP: 
      if (!dataPin()) /* if stop bit not seen */
        kbd_flags |= FLA_RX_BAD;
      if (!(kbd_flags & FLA_RX_BAD)) {
        dataLo();
        kbd_flags |= FLA_RX_BYTE;
      }
      break; /* end clk hi 10 */
    case RX_SENT_ACK: 
      dataHi();
      kbd_state = IDLE_START;
      /* remains in clk hi 11 */
      return;
    }
    clkLo();
    kbd_flags &= ~(FLA_CLOCK_HIGH);
    kbd_state++;
    return;
  }
  else // end < RX_END
  if (kbd_state < TX_END) {
    if (kbd_flags & FLA_CLOCK_HIGH) {
      if (!clkPin()) {
        /* PC is still holding clock down */
        dataHi();
        kbd_state = IDLE_WAIT_REL;
        return;
      }
      kbd_flags &= ~FLA_CLOCK_HIGH;
      clkLo();
      return;
    }

    /* at this point clock is low in preparation to going high */
    kbd_flags |= FLA_CLOCK_HIGH;
    clkHi();
    switch(kbd_state) {
    case TX_START:
      tx_shift = tx_byte;
      parity = 0;
    case TX_DATA1: 	
    case TX_DATA2: 	
    case TX_DATA3: 	
    case TX_DATA4:
    case TX_DATA5: 	
    case TX_DATA6: 	
    case TX_DATA7:
      if (tx_shift & 1) {
        dataHi();
        parity++;
      } 
      else {
        dataLo();
      }
      tx_shift >>= 1;
      break; /* clock hi 1 to 8 */

    case TX_PARITY: 
      if (parity & 1)
        dataLo();
      else
        dataHi();
      kbd_flags &= ~FLA_TX_BYTE;
      kbd_flags |= FLA_TX_OK;
      break; /* clock hi 9 */
    case TX_STOP: 
      dataHi();
      break; /* clock hi 10 */
    case TX_AFTER_STOP:
      kbd_state = IDLE_START;
      /* remains in clk hi 11 */
      return;
    }
    kbd_state++;
  } //else
}

void kbd_set_leds(uint8_t leds)
{
}

void kbd_init(void)
{
  kbd_state = IDLE_START;
  kbd_flags = FLA_CLOCK_HIGH | FLA_TX_OK;

  clkHi();
  dataHi();
}

void kbd_set_tx(unsigned char txchar)
{
  tx_byte = txchar;
  noInterrupts();
  kbd_flags &= ~FLA_TX_OK;
  kbd_flags |= FLA_TX_BYTE;
  interrupts();
}

unsigned char kbd_get_rx_char(void)
{
  noInterrupts();
  kbd_flags &= ~FLA_RX_BYTE;
  interrupts();
  return rx_byte;
}

#define STA_NORMAL		0
#define STA_RXCHAR		1
#define STA_WAIT_SCAN_SET	2
#define STA_WAIT_SCAN_REPLY	3
#define STA_WAIT_ID		4
#define STA_WAIT_ID1		5
#define STA_WAIT_LEDS		6
#define STA_WAIT_AUTOREP	7
#define STA_WAIT_RESET		8
#define STA_DISABLED		9
#define STA_DELAY           11
#define STA_REPEAT        12

#define START_MAKE 0xFF
#define END_MAKE   0xFE
#define NO_REPEAT  0xFD
#define SPLIT      0xFC

// Output buffer - circular queue
#define QUEUE_SIZE 200
static uint8_t QUEUE[QUEUE_SIZE];
static int rear=0, front=0;

static uint8_t lastMAKE_keyidx;
static uint8_t lastMAKE[10];
static uint8_t lastMAKE_SIZE=0;
static uint8_t lastMAKE_IDX=0;
static long loopCnt;

static uint8_t TYPEMATIC_DELAY=2;
static long TYPEMATIC_REPEAT=5;

unsigned char txScanCode = 0; // scancode being urrently transmitted
unsigned char m_state;
unsigned char lastSent;
unsigned char lastState;

// Queue operation -> push, pop
void push(uint8_t item) {
  static uint8_t record=0;

  if(item==START_MAKE) {
    lastMAKE_SIZE=0;
    record=1;
    return;
  }
  if(item==END_MAKE) {
    record=0;
    return;
  }
  if(item==NO_REPEAT) {
    lastMAKE_SIZE=0;
    record=0;
    return;
  }

  if(record)
    lastMAKE[lastMAKE_SIZE++] = item;

  rear = (rear+1)%QUEUE_SIZE;
  if(front==rear) {
    rear = (rear!=0) ? (rear-1):(QUEUE_SIZE-1);
    return;
  }
  QUEUE[rear] = item;
}

uint8_t pop(void) {
  if(front==rear) {
    return 0;
  }
  front = (front+1)%QUEUE_SIZE;

  return QUEUE[front];
}

uint8_t isEmpty(void) {
  if(front==rear)
    return 1;
  else
    return 0;
}

void clear(void) {
  rear = front = 0;
  lastMAKE_SIZE=0;
  lastMAKE_IDX=0;
  loopCnt=0;
  matrix_clear();
}

void tx_state(unsigned char x, unsigned char newstate)
{
  if(x != 0xFE)
    lastSent=x;
  kbd_set_tx(x);
  m_state = newstate;
}

uint8_t keymap_get_keycode(uint8_t layer, uint8_t row, uint8_t col);

void action_exec(uint8_t col, uint8_t row, uint8_t pressed)
{
  static uint8_t layer = 0;
  uint8_t l, scancode;

  l=layer;
  do 
    scancode =  keymap_get_keycode(l, row, col);
  while (l-- && scancode == KC_TRANSPARENT);
  if (scancode == KC_TRANSPARENT || scancode == KC_NO) return;

  if (scancode == KC_FN0) {
    if (pressed) layer = 1;
    else layer = 0;
    return;
  }

  if (pressed) {
    lastMAKE_keyidx = scancode;
    loopCnt=0;
    m_state = STA_NORMAL;

    switch(scancode) {
    case KC_PSCREEN:
      push(START_MAKE);
      push(0xE0);
      push(0x12);
      push(0xE0);
      push(0x7C);
      push(END_MAKE);
      push(SPLIT); // SPLIT is for make sure all key codes are transmitted before disturbed by RX
      break;
    case KC_PAUSE:
      push(NO_REPEAT);
      push(0xE1);
      push(0x14);
      push(0x77);
      push(0xE1);
      push(0xF0);
      push(0x14);
      push(0xF0);
      push(0x77);
      push(SPLIT);
      break;
    case KC_F7:
      push(START_MAKE);
      push(KC_F7);
      push(END_MAKE);
      push(SPLIT);
      break;
    default:
      push(START_MAKE);
      if(scancode & 0x80) push(0xE0);
      push(scancode & 0x7F);
      push(END_MAKE);
      push(SPLIT);
      break;
    }
  } 
  else {
    if(lastMAKE_keyidx == scancode)		// repeat is resetted only if last make key is released
      lastMAKE_SIZE=0;

    switch(scancode) {
    case KC_PSCREEN:
      push(0xE0);
      push(0xF0);
      push(0x7C);
      push(0xE0);
      push(0xF0);
      push(0x12);
      push(SPLIT);
      break;
    case KC_PAUSE:
      break;
    case KC_F7:
      push(0xF0);
      push(KC_F7);
      push(SPLIT);
      break;
    default:	
      if(scancode & 0x80) push(0xE0);
      push(0xF0);
      push(scancode & 0x7F);
      push(SPLIT);
      break;
    }
  }
}


int scankey(void) {
  static matrix_row_t matrix_prev[MATRIX_ROWS];
  static uint8_t led_status = 0;
  matrix_row_t matrix_row = 0;
  matrix_row_t matrix_change = 0;

  matrix_scan();

  //  if (kbd_flags & FLA_RX_BAD) {
  //    cli();
  //    kbd_flags &= ~FLA_RX_BAD;
  //    sei();

  for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
    matrix_row = matrix_get_row(r);
    matrix_change = matrix_row ^ matrix_prev[r];
    if (matrix_change) {
      for (uint8_t c = 0; c < MATRIX_COLS; c++) {
        if (matrix_change & ((matrix_row_t)1<<c)) {
          action_exec(c,r,(matrix_row & ((matrix_row_t)1<<c))!=0);
          /* record a processed key */
          matrix_prev[r] ^= ((matrix_row_t)1<<c);
        }
      }
    }
  }
  return 1;
}

void setup()
{
  Serial.begin(115200);
  matrix_init();
  kbd_init();
  timer2_init();
  m_state = STA_WAIT_RESET;
  Serial.print("INIT COMPLETE");
}


void loop()
{

  static unsigned char rxed;

  static int keyval=0;

  // check that every key code for single keys are transmitted
  if ((kbd_flags & FLA_RX_BYTE) && (keyval==SPLIT || isEmpty())) {
    // pokud nastaveny flag prijmu bytu, vezmi ho a zanalyzuj
    // pokud law, the flag setting apart, take it and zanalyzuj
    rxed = kbd_get_rx_char();		

    switch(m_state) {
    default:
      switch(rxed) {
      case 0xEE: /* echo */  tx_state(0xEE, m_state);  return;
      case 0xF2: /* read id */ tx_state(0xFA, STA_WAIT_ID);  return;
      case 0xFF: /* reset */ tx_state(0xFA, STA_WAIT_RESET);  return;
      case 0xFE: /* resend */  tx_state(lastSent, m_state);  return;
      case 0xF0: /* scan code set */ tx_state(0xFA, STA_WAIT_SCAN_SET);  return;
      case 0xED: /* led indicators */ tx_state(0xFA, STA_WAIT_LEDS);   return;
      case 0xF3:    tx_state(0xFA, STA_WAIT_AUTOREP);  return;
      case 0xF4: /* enable */  break; //tx_state(0xFA, STA_NORMAL/*STA_RXCHAR*/); return;
      case 0xF5: /* disable */  /* clear(); tx_state(0xFA, STA_DISABLED); */
        //tx_state(0xFA, STA_NORMAL); return;
        break;
      case 0xF6: /* Set Default */
        TYPEMATIC_DELAY=2;
        TYPEMATIC_REPEAT=5;
        clear();
      default: 
        break;
      }
      tx_state(0xFA, STA_NORMAL);
      return;

    case STA_RXCHAR:
      if (rxed == 0xF5)
        tx_state(0xFA, STA_NORMAL);
      else {
        tx_state(0xFA, STA_RXCHAR);
      }
      return;

    case STA_WAIT_SCAN_SET:
      clear();
      tx_state(0xFA, rxed == 0 ? STA_WAIT_SCAN_REPLY : STA_NORMAL);
      return;

    case STA_WAIT_LEDS:
      kbd_set_leds(rxed);
      tx_state(0xFA, STA_NORMAL);
      return;

    case STA_WAIT_AUTOREP:
      TYPEMATIC_DELAY = (rxed&0b01100000)/0b00100000;
      int temp_a = (rxed&0b00000111);
      int temp_b = (rxed&0b00011000)/(0b000001000);
      int j=1;
      for(int i=0;i<temp_b;i++)  j = j*2;
      TYPEMATIC_REPEAT = temp_a*j;
      tx_state(0xFA, STA_NORMAL);
      return;    
    }
  }

  if (kbd_flags & FLA_TX_OK) {   // pokud flag odesilani ok -> if the flag sent ok
    switch(m_state) {
    case STA_NORMAL:
      // if error during send
      if(isEmpty())
        scankey();

      keyval=pop();
      if(keyval==SPLIT)return;

      if(keyval) {
        tx_state(keyval, STA_NORMAL);
        loopCnt=0;
      }
      else if(lastMAKE_SIZE>0) {		// means key is still pressed
        loopCnt++;

        // if key is pressed until typmatic_delay, goes to repeat the last key
        if(loopCnt >= TYPEMATIC_DELAY*250+300) {
          loopCnt=0;
          lastMAKE_IDX=0;
          m_state = STA_REPEAT;
        }
      }

      break;

      // typematic : repeat last key
    case STA_REPEAT:
      /* key state can be escaped only if whole key scancode is transmitted */
      if(lastMAKE_IDX==0)
        scankey();

      if(lastMAKE_SIZE==0 || !isEmpty()) { 	/* key is released. go to normal  */
        m_state=STA_NORMAL;
        loopCnt=0;
        break;
      }

      /* if release key is pushed, send them. */
      if(loopCnt==1 || lastMAKE_IDX!=0) {
        tx_state(lastMAKE[lastMAKE_IDX++], STA_REPEAT);
        lastMAKE_IDX %= lastMAKE_SIZE;
      }

      loopCnt++;
      loopCnt %= (3+TYPEMATIC_REPEAT*10);
      break;

    case STA_WAIT_SCAN_REPLY: 
      tx_state(0x02, STA_NORMAL); 
      break;
    case STA_WAIT_ID:  
      tx_state(0xAB, STA_WAIT_ID1); 
      break;
    case STA_WAIT_ID1: 
      tx_state(0x83, STA_NORMAL);  
      break;
      delay(300);
    case STA_WAIT_RESET:
      kbd_set_leds(0xFF);
      clear();
      delay(1000);
      kbd_set_leds(0);
      tx_state(0xAA, STA_NORMAL);
      break;
    }
  }
}





