
static uint8_t matrix_selected;
void init_cols();
void init_ac();
void init_adc();

#define MATRIX_ROWS 8


void init_cols()
{
    // PullUp (DDR:0, PORT:1)
    DDRC  &= ~0b00111111;
    PORTC |=  0b00111111;
    DDRD  &= ~0b01001100; // D6 reference diode pullup
    PORTD |=  0b01001100;
}

void init_ac()
{
    ADCSRB |= (1<<ACME); // analog comp multiplexer enable, AIN1(-)=D7 unused
    ADCSRA &=~(1<<ADEN); // disable ADC to use ADMUX by AC
    ACSR &= ~((1<<ACD)|(1<<ACBG));  // no bangap, external reference  AIN0(+)=D6
}

void init_adc()
{
    ADCSRB |= (1<<ACME); // analog comp multiplexer enable, AIN1(-)=D7 unused
    uint8_t aref = 1; // 0:aref-input, 1:avcc-to-aref 2:reservd 3:aref-to-1.1v
    ADMUX = (ADMUX&0x3f) | aref << 6;
    ADCSRA |= (1<<ADEN); // enable ADC
    ACSR |= (1<<ACD)|(0<<ACBG);  // disable ac, no bangap
}

int adc()
{
	uint8_t low, high;
        uint32_t res;
	ADCSRA|= _BV(ADSC); // start the conversion
	while(ADCSRA&_BV(ADSC)); // ADSC is cleared when the conversion finishes
	low  = ADCL;
	high = ADCH;
	res = (high << 8) | low;
        return (res*625L)>>7;
}

boolean acmp()
{
        return (ACSR & _BV(ACO)) != 0;
}



static void select_row(uint8_t row)
{
    // Output low(DDR:1, PORT:0) to select
	if (row < 6) {
		DDRC  |= 1<<row;
		PORTC &= ~(1<<row);
	} else switch(row) {
		case 6:
		DDRD  |= (1<<3);
		PORTD &= ~(1<<3);
		break;
		case 7:
		DDRD  |= (1<<2);
		PORTD &= ~(1<<2);
		break;
	}
	matrix_selected = row;
}

static void unselect_rows(void)
{
    // PullUp(DDR:0, PORT:1) to unselect
    DDRC  &= ~0b00111111;
//    PORTC &= ~0b00111111;
    PORTC |=  0b00111111;
    DDRD  &= ~0b00001100;
//    PORTD &= ~0b00001100;
    PORTD |=  0b00001100;
}

void setup()
{
  Serial.begin(115200);
  init_cols();
  init_ac();
//  init_adc();
  Serial.println("matrix_init()");
}


void loop()
{
    char buf[8];
    Serial.println("-------");
    for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
        Serial.print(i);  Serial.print(':');
        select_row(i);
        for(uint8_t j=0; j < MATRIX_ROWS; j++) {
		ADMUX = (ADMUX & 0xF0) | (j & 0x07);
//                if (j==i) continue;
//		if (j < 6) PORTC |= (1<<j);
//		else if (j==6) PORTD |= (1<<3);
//		else if (j==7) PORTD |= (1<<2);
		delayMicroseconds(5);
        	sprintf(buf,"%5d",acmp());
//        	sprintf(buf,"%5d",adc());
        	Serial.print(buf);
//		if (j < 6) PORTC &= ~(1<<j);
//		else if (j==6) PORTD &= ~(1<<3);
//		else if (j==7) PORTD &= ~(1<<2);
        }
        unselect_rows();
        Serial.println();
    }
    delay(250);
}

