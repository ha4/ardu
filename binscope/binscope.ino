#ifdef F_CPU
#define SYSCLOCK  F_CPU
#else
#define SYSCLOCK  16000000
#endif
#define USECPERTICK    50

#define IR_USE_TIMER2     // tx = pin 3

#if defined(IR_USE_TIMER2)
#define TIMER_RESET
#define TIMER_ENABLE_PWM    (TCCR2A |= _BV(COM2B1))
#define TIMER_DISABLE_PWM   (TCCR2A &= ~(_BV(COM2B1)))
#define TIMER_ENABLE_INTR   (TIMSK2 = _BV(OCIE2A))
#define TIMER_DISABLE_INTR  (TIMSK2 = 0)
#define TIMER_INTR_NAME     TIMER2_COMPA_vect
#define TIMER_CONFIG_KHZ(val) ({ \
	const uint8_t pwmval = SYSCLOCK / 2000 / (val); \
	TCCR2A               = _BV(WGM20); \
	TCCR2B               = _BV(WGM22) | _BV(CS20); \
	OCR2A                = pwmval; \
	OCR2B                = pwmval / 3; \
})
#define TIMER_COUNT_TOP  (SYSCLOCK * USECPERTICK / 1000000)
#if (TIMER_COUNT_TOP < 256)
#	define TIMER_CONFIG_NORMAL() ({ \
		TCCR2A = _BV(WGM21); \
		TCCR2B = _BV(CS20); \
		OCR2A  = TIMER_COUNT_TOP; \
		TCNT2  = 0; \
	})
#else
#	define TIMER_CONFIG_NORMAL() ({ \
		TCCR2A = _BV(WGM21); \
		TCCR2B = _BV(CS21); \
		OCR2A  = TIMER_COUNT_TOP / 8; \
		TCNT2  = 0; \
	})
#endif
#endif

enum {inpin = 11};
uint8_t bits, nbits;

ISR (TIMER_INTR_NAME)
{
	TIMER_RESET;
        uint8_t  irdata = (uint8_t)digitalRead(inpin);
        if (nbits == 8) { Serial.write(bits);  bits=0; nbits=0; }
        bits = (bits << 1) | irdata;
        nbits++; 
}

void  enableIn ( )
{
	cli();
        bits = 0; nbits=0;
	TIMER_CONFIG_NORMAL();
	TIMER_ENABLE_INTR;
	TIMER_RESET;
	sei();  // enable interrupts
	pinMode(inpin, INPUT);
}

void setup()
{
  Serial.begin(115200);
  Serial.print('A');
  enableIn();
}

void loop()
{
}

