
// serial access
void ser_init(uint16_t bittime);
// unbuffered
uint8_t ser_read();
uint8_t ser_available();
//uint16_t ser_parseInt();
void ser_write(uint8_t data);
void ser_print(char *s);
void ser_printInt(uint16_t n);
//void ser_printLong(uint32_t n);
void ser_println();

void adc_init(uint8_t amode);
//uint16_t adc_read();
uint16_t adc_get();

#define adc_start()   ADCSRA|= _BV(ADSC)


void timer_init(uint8_t tmode);
unsigned long ticks();
void timer_pwm(uint8_t x);
