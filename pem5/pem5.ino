
// freq meter
uint32_t volatile timerc0;
uint32_t volatile timerc1;
uint16_t volatile timerp1;

void setup()
{
  Serial.begin(9600);

  // timer2 clock output
  // PD3, OC2B, D3 pin 1MHz clock
  //       mode 010 (CTC),top=OCR2A, COM2B mode=1 (toggle), src=001
  TCCR2A = (0b01 << COM2B0)| (0b10 << WGM20); // div 2
  TCCR2B = (0b00 << WGM22) | (0b001 << CS20); // clk/1 (16MHz)
  OCR2A = 4 - 1; // div 4
  OCR2B = 0;
  pinMode(3, OUTPUT);

  // timer0 interrupt
  OCR0B = 100; // every 256 clk/precaler interrupt
  TIFR0 |= 1 << OCF0B; // clear flag
  TIMSK0 |= 1<<OCIE0B; // set COMP-B interrupts

  // timer1 freqmeter clock source T1 (PIN5)
  TCCR1A = 0;
  TCCR1B = 0b111 << CS10; // 0b111 external T1, rising edge 0x
  //TCCR1B = 0b001 << CS10; // 0b001 clk/1
  // clear timer1
  TCNT1H = 0; // latch high part
  TCNT1L = 0;
}

SIGNAL(TIMER0_COMPB_vect)
{
  uint8_t low;
  uint16_t m,d;

  low = TCNT1L; // catch high byte
  m = (TCNT1H << 8) | low;
  d = m - timerp1;
  timerp1=m;
  timerc1 += d;
  timerc0++;
}

uint32_t freq_t1()
{
  uint8_t oldSREG = SREG;
  uint32_t a,b;
  uint64_t f;
  cli();
  if (timerc0 >= 976) { // 976.5625 hz = 16mhz/64/256 = 15625/16
    a=timerc1;
    b=timerc0;
    timerc1=timerc0=0;
    SREG = oldSREG;
    f = a;
    f *= 1000000;
    f /= b*1024;
    return f;
  } else {
    SREG = oldSREG;
    return 0xFFFFFFFF;
  }
}

void loop()
{
  unsigned long frequency;
  
  frequency = freq_t1();
  if (frequency!=0xFFFFFFFF) {
    Serial.println(frequency);
  }
}
