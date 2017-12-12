
/*
 * scan matrix
 */
#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>
//#include <avr/io.h>
//#include <util/delay.h>

#include "config.h"
#include "matrix.h"


#ifndef DEBOUNCE
#   define DEBOUNCE	5
#endif

static uint8_t debouncing = DEBOUNCE;

/* matrix state(1:on, 0:off) */
static matrix_row_t matrix[MATRIX_ROWS];
static matrix_row_t matrix_debouncing[MATRIX_ROWS];
static uint8_t matrix_selected;

static matrix_row_t read_cols(void);
static void init_cols(void);
static void unselect_rows(void);
static void select_row(uint8_t row);


void matrix_init(void)
{
    // initialize row and col
    unselect_rows();
    init_cols();
    matrix_clear();
}

uint8_t matrix_scan(void)
{
    for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
        select_row(i);
        matrix_row_t cols = read_cols();
        if (matrix_debouncing[i] != cols) {
            matrix_debouncing[i] = cols;
            debouncing = DEBOUNCE;
        }
        unselect_rows();
    }

    if (debouncing) {
        if (--debouncing)
            delay(1);
        else 
            for (uint8_t i = 0; i < MATRIX_ROWS; i++)
                matrix[i] = matrix_debouncing[i];
    }

    return 1;
}

matrix_row_t matrix_get_row(uint8_t row)
{
    return matrix[row];
}

/* Pin configuration: 
   A7+D2, A6+D3 connectted
 * row: 0   1   2   3   4   5   6   7
 * pin: C0  C1  C2  C3  C4  C5  D3  D2
 */

static void  init_cols(void) {
    // PullUp (DDR:0, PORT:1)
    DDRC  &= ~0b00111111;
    PORTC |=  0b00111111;
    DDRD  &= ~0b01001100; // D6 reference diode pullup
    PORTD |=  0b01001100;

    ADCSRB |= (1<<ACME); // analog comp multiplexer enable, AIN1(-)=D7 unused
    ADCSRA &=~(1<<ADEN); // disable ADC to use ADMUX by AC
    ACSR &= ~((1<<ACD)|(1<<ACBG));  // no bangap, external reference  AIN0(+)=D6
}

static void unselect_rows(void)
{
    // PullUp(DDR:0, PORT:1) to unselect
    DDRC  &= ~0b00111111;
    PORTC |=  0b00111111;
    DDRD  &= ~0b00001100;
    PORTD |=  0b00001100;
}

static matrix_row_t read_cols(void)
{
    uint8_t r = 0;
    for(byte j=0; j < MATRIX_ROWS; j++) {
      ADMUX = (ADMUX & 0xF0) | (j & 0x07);
      delayMicroseconds(5);
      if (j != matrix_selected) { 
        r >>= 1;
        if (ACSR & (1<<ACO)) r |= 0x80;
      }
    }
    return r>>1;
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


uint8_t matrix_rows(void) { return MATRIX_ROWS; }
uint8_t matrix_cols(void) { return MATRIX_COLS; }

void matrix_clear(void)
{
    // initialize matrix state: all keys off
    for (uint8_t i=0; i < MATRIX_ROWS; i++) {
        matrix[i] = 0;
        matrix_debouncing[i] = 0;
    }
}

void matrix_setup(void) { }

void matrix_power_up(void) { }
void matrix_power_down(void) { }

bool matrix_is_on(uint8_t row, uint8_t col)
{
    return (matrix_get_row(row) & (1<<col));
}

void matrix_print(void)
{
/*	for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
		uint8_t b=matrix_get_row(row);
		for(uint8_t i=0, h=1; i < MATRIX_COLS; i++, h<<=1) {
		    Serial.print((b&h)!=0);
		}
		Serial.println();
	}
*/
}
