
#define SBAUD (F_CPU/9600L)
#define SPIN PINB
#define SPORT PORTB
#define SDDR DDRB

// PB1 #6 miso
#define STXBIT _BV(1)
// PB2 #7 sck
//#define SRXBIT _BV(2)
// PB3 #2
//#define SRXBIT _BV(3)
// PB4 #3
#define SRXBIT _BV(4)
// PB0 #5 oc0a/mosi
#define PWMBIT _BV(0)

#define T0_DEFAULT 0
#define T0_COMPA _BV(OCIE0A)
#define T0_COMPB _BV(OCIE0B)
#define TMODE T0_COMPB
#define TCLKSEL (0b101)
#define TODIV 1024
#define USEC2TICK(us) (us*F_CPU/TODIV/1000000L)

//
// ADC
//
#define ADC_START 0x00
#define ADC_INTR _BV(ADIE)
#define AMODE ADC_INTR

// 5v reference
//#define AREFERENCE 0x00
// internal 1.1v  reference
#define AREFERENCE _BV(REFS0)

// PB4 #1 ADC0 nRST
//#define ACHANNEL 0x00
// PB2 #7 ADC1 sck
//#define ACHANNEL 0x01
// PB3 #2 ADC3
#define ACHANNEL 0x03
// PB4 #3 ADC2
//#define ACHANNEL 0x02

// PB4 (+) PB3(-) x1
//#define ACHANNEL 0x06

// PB4 (+) PB3(-) x20
//#define ACHANNEL 0x07
