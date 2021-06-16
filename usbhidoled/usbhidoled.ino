#include <HID.h>

extern void graph_setup();
extern void graph_loop();

/* HID Class Report Descriptor */
/* Short items: size is 0, 1, 2 or 3 specifying 0, 1, 2 or 4 (four) bytes */
/* of data as per HID Class standard */
 
/* Main items */
#define INPUT(size)             (0x80 | size)
#define OUTPUT(size)            (0x90 | size)
#define FEATURE(size)           (0xb0 | size)
#define COLLECTION(size)        (0xa0 | size)
#define END_COLLECTION(size)    (0xc0 | size)
 
/* Global items */
#define USAGE_PAGE(size)        (0x04 | size)
#define LOGICAL_MINIMUM(size)   (0x14 | size)
#define LOGICAL_MAXIMUM(size)   (0x24 | size)
#define PHYSICAL_MINIMUM(size)  (0x34 | size)
#define PHYSICAL_MAXIMUM(size)  (0x44 | size)
#define UNIT_EXPONENT(size)     (0x54 | size)
#define UNIT(size)              (0x64 | size)
#define REPORT_SIZE(size)       (0x74 | size)
#define REPORT_ID(size)         (0x84 | size)
#define REPORT_COUNT(size)      (0x94 | size)
#define PUSH(size)              (0xa4 | size)
#define POP(size)               (0xb4 | size)
 
/* Local items */
#define USAGE(size)                 (0x08 | size)
#define USAGE_MINIMUM(size)         (0x18 | size)
#define USAGE_MAXIMUM(size)         (0x28 | size)
#define DESIGNATOR_INDEX(size)      (0x38 | size)
#define DESIGNATOR_MINIMUM(size)    (0x48 | size)
#define DESIGNATOR_MAXIMUM(size)    (0x58 | size)
#define STRING_INDEX(size)          (0x78 | size)
#define STRING_MINIMUM(size)        (0x88 | size)
#define STRING_MAXIMUM(size)        (0x98 | size)
#define DELIMITER(size)             (0xa8 | size)

/* HID PAGES */
#define HID_GENERIC_DESKTOP  0x01
#define HID_BUTTON  0x09

/* HID USAGES */
#define HID_GAME_PAD 0x05
#define HID_X 0x30
#define HID_Y 0x31
#define HID_Z 0x32
#define HID_RX 0x33
#define HID_RY 0x34
#define HID_RZ 0x35

/* COLLECTIONS */
#define C_PHYSICAL 0x00
#define C_APPLICATION 0x01
#define C_LOGICAL 0x02
#define C_VENDOR_DEFINED 0x02

/* INPUT/OUTPUT/FREATURE */
#define T_DATA  0x00
#define T_CONST 0x01
#define T_ARRAY 0x00
#define T_VARIABLE  0x02
#define T_ABSOLUTE 0x00
#define T_RELATIVE 0x04
#define T_NOWRAP   0x00
#define T_WRAP     0x08
#define T_LINEAR   0x00
#define T_NLIN     0x10
#define T_PREFSTATE 0x00
#define T_NPRF      0x20
#define T_NONULL    0x00
#define T_NULLSTATE 0x40
#define T_NONVOLATILE 0x00
#define T_VOLATILE  0x80
#define T_BITFIELD  0x00
#define T_BUFFERED  0x00, 0x01

const uint8_t _hid_rpt_descr[] PROGMEM = {
  USAGE_PAGE(1), HID_GENERIC_DESKTOP,
  USAGE(1), HID_GAME_PAD,
  COLLECTION(1), C_APPLICATION,
    REPORT_ID(1), 0x03,
    COLLECTION(1), C_PHYSICAL,
    USAGE(1), HID_X,
    USAGE(1), HID_Y,
    USAGE(1), HID_RX,
    USAGE(1), HID_RY,
    LOGICAL_MINIMUM(2), 0x01, 0x80, // -32767
    LOGICAL_MAXIMUM(2), 0xff, 0x7f,  // 32767
    REPORT_SIZE(1), 16,
    REPORT_COUNT(1), 4,
    INPUT(1), T_DATA|T_VARIABLE|T_ABSOLUTE,
    END_COLLECTION(0),

    USAGE_PAGE(1), HID_BUTTON,
    USAGE_MINIMUM(1), 0x01, //  (Button 1)
    USAGE_MAXIMUM(1), 0x08, //  (Button 8)
    LOGICAL_MINIMUM(1), 0,
    LOGICAL_MAXIMUM(1), 1,
    REPORT_SIZE(1), 1,
    REPORT_COUNT(1), 8,
    INPUT(1), T_DATA|T_VARIABLE|T_ABSOLUTE,
  END_COLLECTION(0)
};

struct usb_gamepad_rpt {
  int16_t leftX;
  int16_t leftY;
  int16_t rightX;
  int16_t rightY;
  uint8_t buttons;
};  // 9 bytes

struct usb_gamepad_rpt rpt_data;


class xtimer {
public:
  xtimer() { stop(); }
  ~xtimer() { }

  byte check(uint32_t t) {
    if (wait==0xFFFFFFFF) { t0 = t; return 0; }
    if (wait==0) { t0 = t; return 1; }
    if (t - t0 < wait)  return 0;
    t0 = t;
    return 1;
  }

  void set(uint32_t dt) { wait=dt; }
  void start(uint32_t now) { t0=now; }
  void stop() { set(0xFFFFFFFF); }

  uint32_t t0;
  uint32_t wait;
};

xtimer potimer; // Pot read
xtimer dtimer; // display
xtimer prtimer; // print serial



void mygamepad_init(void) {
  static HIDSubDescriptor node(_hid_rpt_descr, sizeof(_hid_rpt_descr));
  HID().AppendDescriptor(&node);
}

void send_report()
{
  HID().SendReport(3, &rpt_data, sizeof(rpt_data));
}

#define TEST_MAX 4095
#define TEST_SPEED 512
#define TEST_SPEED2 1024
#define TEST_FACTR 8 

int  limadd(int a, int b) { a+=b;  return max(-TEST_MAX,min(a,TEST_MAX)); }

bool test_rpt()
{
    static int sa=0, ca=TEST_MAX;
    static int sb=TEST_MAX, cb=0;
    int ts, tc;

    rpt_data.leftX=sa*TEST_FACTR;
    rpt_data.leftY=ca*TEST_FACTR;
    rpt_data.rightX=sb*TEST_FACTR;
    rpt_data.rightY=cb*TEST_FACTR;
    rpt_data.buttons = (sa < 0 ? 1 : 0) | (((ca < 0)^(sa < 0)) ? 2 : 0);

    ts=sa/TEST_SPEED;  tc=ca/TEST_SPEED;  sa=limadd(sa,tc); ca=limadd(ca,-ts);
    ts=sb/TEST_SPEED2; tc=cb/TEST_SPEED2; sb=limadd(sb,tc); cb=limadd(cb,-ts);

    return true;
}

#define ADC_SIZE 4

#define ADC_CONFIG  _BV(ADEN) | _BV(ADSC) | _BV(ADATE) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0)
#define ADC_REFSEL  _BV(REFS0) /*| _BV(REFS1)*/

#define CHAN_NEXT(n) (((n)>=(ADC_SIZE-1))?0:(n)+1)

static volatile uint8_t adc_chan, adc_next, adc_prev;
static volatile uint16_t adc_values[ADC_SIZE];

uint8_t adc_chan_mux(uint8_t n)
{
  return 7-n;
}

ISR(ADC_vect)
{
#if defined(__AVR_ATmega32U4__)
/*
 * 0x07 ADC7 = A0 0
 * 0x06 ADC6 = A1 1
 * 0x05 ADC5 = A2 2
 * 0x04 ADC4 = A3 3
 * 0x01 ADC1 = A4 4
 * 0x00 ADC0 = A5
 * 0x1E BANDGAP   5
 * 0x1F GND
 */
  adc_values[adc_prev] = ADC;
  ADMUX = adc_next;
  adc_prev = adc_chan;
  adc_chan = CHAN_NEXT(adc_chan);
  adc_next = adc_chan_mux(CHAN_NEXT(adc_chan)) | ADC_REFSEL;
#endif
}

uint16_t adc_read(uint8_t chan)
{
  uint16_t v;
  noInterrupts();
  v = adc_values[chan];
  interrupts();
  return v;
}

void adc_setup()
{
  // 125khz (ADPS=0b111, 16MHz div128)
  noInterrupts();
  adc_prev = 0;
  adc_chan = 0;
  adc_next = adc_chan_mux(CHAN_NEXT(adc_chan)) | ADC_REFSEL;
  ADMUX  = adc_chan_mux(adc_chan);
  ADCSRA = ADC_CONFIG;
  ADCSRB = _BV(ADHSM);
  DIDR0  = 0xF0; // digital input disable channel 4..7
  interrupts();
}

bool adc_scanall()
{
  bool c = 0;

  uint8_t b = 0;
  int16_t lx = adc_read(0);
  int16_t ly = adc_read(1);
  int16_t rx = adc_read(2);
  int16_t ry = adc_read(3);
//  lx-=512;  ly-=512;  rx-=512;  ry-=512; // +/-511 scale
//  lx*=64; ly*=64; rx*=64; ry*=64; // +/-32767 scale
  lx*=32; ly*=32; rx*=32; ry*=32; // +/-32767 scale
//  lx*=4; ly*=4; rx*=4; ry*=4; // +/-2047 scale
//  lx+=1000; ly+=1000; rx+=1000; ry+=1000; // 1000..2000 pulse scale
//  lx/=4; ly/=4; rx/=4; ry/=4; // +/-127 scale
//  lx*=2; ly*=2; rx*=2; ry*=2; // 0..2048 scale

  if (b != rpt_data.buttons || 
      ry != rpt_data.rightY || rx != rpt_data.rightX ||
      ly != rpt_data.leftY || lx != rpt_data.leftX) {
    rpt_data.leftX = lx;
    rpt_data.leftY = ly;
    rpt_data.rightX = rx;
    rpt_data.rightY = ry;
    rpt_data.buttons = b;
    c = 1;
  }

  return c;
}

bool comm_serial()
{
  static bool sertostart = 1;
    if (Serial.dtr()) { // is port open
      if (sertostart) {
        sertostart = 0;
        Serial.begin(115200);
      }
      return 1;
    } else if (!sertostart) {
      sertostart = 1;
      Serial.end();
    }
    return 0;
}

void setup() {
  uint32_t t;

  t = micros();
  mygamepad_init();
  memset(&rpt_data, 0, sizeof(rpt_data));
  potimer.start(t);
  potimer.set(1000);
  dtimer.start(t);
  dtimer.set(100000L);
  prtimer.start(t);
  prtimer.set(100000UL);
  adc_setup();
  graph_setup();
}

void loop() {
  static uint32_t dt1,dt2;
  uint32_t t = micros();
  if(potimer.check(t)) {
    if (adc_scanall()) send_report();
    dt1=micros()-t;
  }
  if(dtimer.check(t)) {
    graph_loop();
    dt2=micros()-t;
  }
  if(prtimer.check(t) && comm_serial()) {
    Serial.print("dt1:");Serial.print(dt1);
    Serial.print(" dt2:");Serial.println(dt2);
  }
}
