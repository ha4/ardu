
#define SUPPORT_CALIBRATION	(0)
/* set nonzero to flash LED when it changes range */
#define RANGE_DEBUG /* SetOutputOn() */
#define ACTIVE_LOW  (1)

#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
	
/* 
  Pin assignments tiny25:
   PB0/MOSI     #5   far LED drive, active low
   PB1/MISO     #6   near LED drive, active low
   PB2/ADC1/SCK #7   output to Duet via 3K6 (12K) resistor
   PB3/ADC3     #2   output to Duet via 3K (10K) resistor
   PB4/ADC2     #3   input from phototransistor
   PB5/ADC0/RESET #1 not available, used for programming
   VCC,GND     #8,#4 decouple 0.1u
*/

#ifndef __ECV__
/* fuse for attiny */
//__fuse_t __fuse __attribute__((section (".fuse"))) = {0xE2u, 0xDFu, 0xFFu};
#endif

#define PortOUT PORTB
#define PortDIR DDRB
#define RegTI  TIFR
#define RegTM  TIMSK

const unsigned int AdcPhototransistorChan = 2;	/*  ADC channel for the phototransistor */
const unsigned int AdcDuet3K0OutputChan = 3; /* ADC channel for the 3K0 and LED output bit, when we use it as an input */
const unsigned int AdcDuet3K6OutputChan = 1; /* ADC channel for the 3K6 output bit, when we use it as an input */
const unsigned int NearLedBit = 1;
const unsigned int FarLedBit = 0;
const unsigned int Duet3K0OutputBit = 3;
const unsigned int Duet3K6OutputBit = 2;

const uint8_t PortUnusedBitMask = 0;

#define REFVCC 0b00000000
#define REF1V1 0b10000000

#define IS_HIGHSENSE(reg) ((reg & (1 << REFS1)) != 0)

/* prescaler = 64 (8Mhz/128 ADC clock ~= 125kHz) */
#define ADC_PS	_BV(ADPS2) | _BV(ADPS1)

#else

/* Arduino Uno */
#define PortOUT PORTD
#define PortDIR DDRD
#define RegTI  TIFR0
#define RegTM  TIMSK0

const unsigned int AdcPhototransistorChan = 0;	/* A0, D14, PC0 ADC channel for the phototransistor */
const unsigned int AdcDuet3K0OutputChan = 2; /* ADC channel for the 3K0 and LED output bit, when we use it as an input */
const unsigned int AdcDuet3K6OutputChan = 1; /* ADC channel for the 3K6 output bit, when we use it as an input */
const unsigned int NearLedBit = 4;
const unsigned int FarLedBit = 3;
const unsigned int Duet3K0OutputBit = 5;
const unsigned int Duet3K6OutputBit = 6;

const uint8_t PortUnusedBitMask = 0;

#define REFVCC 0b01000000
#define REF1V1 0b11000000

#define IS_HIGHSENSE(reg) ((reg & (1 << REFS1)) != 0)

/* prescaler = 128 (16Mhz/128 ADC clock ~= 125kHz) */
#define ADC_PS	_BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0)

#endif


#if ACTIVE_LOW
#define PortOn(b) PortOUT &= ~_BV(b)
#define PortOff(b) PortOUT |= _BV(b)
#else
#define PortOn(b) PortOUT |= _BV(b)
#define PortOff(b) PortOUT &= ~_BV(b)
#endif


/* Give a G31 reading of about 0 */
inline void
SetOutputOff()
{
	PortOUT &= ~_BV(Duet3K0OutputBit);
	PortOUT &= ~_BV(Duet3K6OutputBit);
}

/* Give a G31 reading of about 445 indicating we are approaching the trigger point */
inline void
SetOutputApproaching()
{
 	PortOUT &= ~_BV(Duet3K0OutputBit);
	PortOUT |= _BV(Duet3K6OutputBit);
}

/* Give a G31 reading of about 578 indicating we are at/past the trigger point */
inline void
SetOutputOn()
{
	PortOUT |= _BV(Duet3K0OutputBit);
	PortOUT &= ~_BV(Duet3K6OutputBit);
}

/* Give a G31 reading of about 1023 indicating that the sensor is saturating */
inline void
SetOutputSaturated()
{
	PortOUT |= _BV(Duet3K0OutputBit);
	PortOUT |= _BV(Duet3K6OutputBit);
}

/*
  interrupt frequency. We run the IR sensor at one quarter of this, i.e. 2kHz
  highest usable value is about 9.25kHz because the ADC needs 13.5 clock cycles per conversion.
*/
const uint16_t interruptFreq = 8000;
const uint16_t divisorIR = (uint16_t)(F_CPU/interruptFreq);
const uint16_t prescalerIR = 8;
const uint16_t baseTopIR = (divisorIR/prescalerIR) - 1u;
const uint16_t cyclesAveragedIR = 8;
const uint8_t  cyclesAveragedMASK = 0x07;

const uint16_t farThr       =  10u * cyclesAveragedIR; /* minimum far reading for us to think the sensor is working properly */
const uint16_t saturatedThr = 870u * cyclesAveragedIR; /* minimum reading for which we consider the sensor saturated */
const uint16_t rangeUpThr   = 700u * cyclesAveragedIR; /* must be less than saturatedThreshold */
const uint16_t rangeDownThr = 120u * cyclesAveragedIR; /* must be less than about 1/5 rangeUpThreshold */
const uint16_t lowThr =   50u * 3u * cyclesAveragedIR; /* not connected threshold */
const uint16_t hiThr  =  950u * 3u * cyclesAveragedIR; /* connected to VCC threshold */
const uint16_t fullThr= 1023u * 3u * cyclesAveragedIR; /* fullscale threshold */
/* We are looking for a pullup resistor of no more than 160K on the output with 3K to GND
  to indicate that we should use a digital output */
const uint16_t digThr = (fullThr * 3UL)/(160UL + 3UL);

const uint16_t ledFlashTime = interruptFreq/6u;

class IrData
{
public:
	uint16_t readings[cyclesAveragedIR];
	volatile uint16_t sum;
	uint8_t index;

	void addReading(uint16_t arg)
	{
		index = (index + 1) & cyclesAveragedMASK;
		sum = sum - readings[index] + arg;
		readings[index] = arg;
	}
	
	void init()
	{
		for (uint8_t i = 0; i < cyclesAveragedIR; ++i)
			readings[i] = 0;
		sum = 0;
	}
	IrData() {}
};

IrData nearData, farData, offData;

	
/*
  General variables
 */
 
/* counts system ticks, lower 2 bits also used for ADC/LED state */
volatile uint16_t tickCounter = 0;

bool digitalOutput = false;
/* volatile because we care about when it is written */
volatile bool running = false;

const uint16_t eepromMagic = 0x52A6;
uint16_t nvData[3];


/*
  EEPROM access functions
 */
void readEEPROM(uint8_t ucAddress, uint8_t *p, uint8_t len)
{
	do {
		/* Wait for completion of previous write */
		while(EECR & (1<<EEPE));
		EEAR = ucAddress++;
		EECR |= (1<<EERE);
		*p++ = EEDR;
	}while(--len != 0);
}

void writeEEPROM(uint8_t ucAddress, const uint8_t *p, uint8_t len)
{
	do {
		/* Wait for completion of previous write */
		while(EECR & (1<<EEPE));
		EECR = (0<<EEPM1)|(0<<EEPM0);
		/* Set up address and data registers */
		EEAR = ucAddress++;
		EEDR = *p++;
		EECR |= (1<<EEMPE);
		EECR |= (1<<EEPE);
	}while(--len != 0);
}


/*
  ISR for the timer 0 compare match interrupt
  This uses 19 bytes of stack (from assembly listing, 2016-07-30)
 */
ISR(TIMER0_COMPA_vect)
{
	/*  while((ADCSRA & ADIF) == 0); */
	const uint16_t adcVal = ADC & 0x3FF;
	const uint8_t locTickCounter = (uint8_t)tickCounter;

	/*
	  delay a little until the ADC s/h has taken effect.
	  3 ADC clocks should be enough, and 1 ADC clock is 8 timer 0 clocks.
	*/
	while(TCNT0 < 3*8);

	switch(locTickCounter & 0x03u) {
		case 0: /* Far LED is on, we just did no reading, we are doing a far reading now and an off reading next */
			PortOff(FarLedBit);
			break;
		
		case 1: /* LEDs are off, we just did a far reading, we are doing a off reading now and a near reading next */			
			PortOn(NearLedBit);
			if(running)
				farData.addReading(adcVal);
			break;
					
		case 2: /* Near LED is on, we just did an off reading, we are doing a near reading now and a dummy off reading next */
			PortOff(NearLedBit);
			if(running)
				offData.addReading(adcVal);
			break;

		case 3: /* Far LED is on, we just did an off reading, we are doing another off reading now which will be discarded */
			PortOn(FarLedBit);
			if(running)
				nearData.addReading(adcVal);
			break;
	}
	++tickCounter;
}


/*
  Get the tick counter from outside the ISR.
  As it's more than 8 bits long, we need to disable interrupts while fetching it.
 */
inline uint16_t
GetTicks()
{
	noInterrupts();
	const uint16_t ticks = tickCounter;
	interrupts();
	return ticks;
}

/* Check whether we need to kick the watchdog */
inline void 
KickWatchdog()
{
}

/* Delay, keeping the watchdog happy */
void
DelayTicks(uint16_t ticks)
{
	const uint16_t startTicks = GetTicks();
	do 
		KickWatchdog();
	while(GetTicks() - startTicks < ticks);
}

uint16_t delta(uint16_t v, uint16_t offset)
{
	return (v > offset) ? v - offset : 0;
}

uint16_t
getTotal()
{
	noInterrupts();
	const uint16_t tot = offData.sum + nearData.sum + farData.sum;
	interrupts();
	return tot;
}

void
clearAll()
{
	running = false;
	nearData.init();
	farData.init();
	offData.init();
}

uint8_t
calibrate()
{
	uint16_t locNearSum, locFarSum, locOffSum; /* Get local copies of volatile variables to save code space */

	/*
	  Output pin has been shorted to Vcc at the correct times, so enter calibration mode
	  Clear out the data and start collecting data from the phototransistor
	 */
	clearAll();

	/* tell interrupt handler to collect readings, wait until we have taken two full sets of readings */
	for(;;){
		running = true;	
		DelayTicks(8u * cyclesAveragedIR + 4);
		running = false;
		
		locNearSum = nearData.sum;
		locFarSum = farData.sum;
		locOffSum = offData.sum;
		
		if(locNearSum >= rangeUpThr || locFarSum >= rangeUpThr){
			/* switch to low sensitivity (voltage reference is VCC) */
			ADMUX = (uint8_t)AdcPhototransistorChan | (REFVCC);
		}
		else
			break;
	}

	const uint16_t nearSum = delta(locNearSum, locOffSum);
	const uint16_t farSum = delta(locFarSum, locOffSum);
	if(farSum < farThr ||
	   locNearSum >= saturatedThr || locFarSum >= saturatedThr ||
	   nearSum > 2 * farSum || farSum > nearSum){
		SetOutputOn();	/* light LED to signal calibration error */
		for(;;) KickWatchdog();
	}
	/*
	  the far LED is stronger than the near one, so to avoid false triggering we
	  don't want the far reading to exceed the near one
	  Successful calibration so set multipliers and save to EEPROM
	 */
	nvData[0] = farSum;
	nvData[1] = nearSum;
	nvData[2] = nvData[0] ^ nvData[1] ^ eepromMagic;
	writeEEPROM(0, reinterpret_cast<const uint8_t*>(&nvData), sizeof(nvData));
			
	return 6u; /* six flashes indicate successful calibration */
}

void
check_mode()
{
  	uint8_t flashesToGo;
	uint16_t totalSum;

  	/*
	  Determine whether to provide a digital output or a 4-state output, or to calibrate the sensor.
	  We do this by checking to see whether the connected electronics provided a pullup resistor on the output.
	  If a pullup resistor is detected, we provide a digital output, else we provide an analog output.
	  Wait a while before we do this test, so that Duet firmware has a chance to turn the internal pullup (50K to 150K) off,
	  and Arduino/RAMPS firmware has a chance to turn the internal pullup (20K to 50K) on.
	 */
	ADMUX = (uint8_t)AdcDuet3K6OutputChan | (REFVCC); /* select the 3K6 resistor output bit, single ended mode */
	SetOutputOff();
	PortDIR &= ~_BV(Duet3K6OutputBit);	/* set the pin to an input, pullup disabled because output is off */
	DelayTicks(4u);
	running = true;	/* start collecting readings */

	/* Wait 1 second to allow voltages to stabilize */
	DelayTicks(1u * interruptFreq);
	totalSum = getTotal();
	bool inProgrammingSeq = totalSum >= hiThr; /* see if output high after 1 second */

	DelayTicks(2u * interruptFreq);
	totalSum = getTotal();
	if(inProgrammingSeq)
		inProgrammingSeq = totalSum <= lowThr; /* see if output low after 3 seconds */

	DelayTicks(2u * interruptFreq);
	running = false;			/* stop collecting readings */
	totalSum = getTotal();
	
	/* Change back to normal operation mode, set the pin back to being an output */
	ADMUX = (uint8_t)AdcPhototransistorChan | (REF1V1);
	PortDIR |= _BV(Duet3K6OutputBit);

	if(inProgrammingSeq && totalSum >= hiThr)
		flashesToGo = calibrate();
	else{
		digitalOutput = totalSum >= digThr;
		flashesToGo = (digitalOutput) ? 2u : 4u;
		
		/* Read multipliers from EEPROM */
		readEEPROM(0u, reinterpret_cast<uint8_t*>(&nvData), sizeof(nvData));
		if(nvData[0] ^ nvData[1] ^ nvData[2] ^ eepromMagic){
			nvData[0] = nvData[1] = 1;
			++flashesToGo;	/* extra flash to indicate not calibrated */
		}	
	}
	
	/* Flash the LED the appropriate number of times */
	while(flashesToGo != 0u){
		SetOutputSaturated();
		DelayTicks(ledFlashTime);
		SetOutputOff();
		DelayTicks(ledFlashTime);
		--flashesToGo;
	}
}

/* Main program */
void
setup(void)
{
	noInterrupts();
	/* disable digital input buffers on ADC inputs */
	DIDR0 = _BV(AdcPhototransistorChan) | _BV(Duet3K6OutputBit);

	/* Set ports and pullup resistors */
	PortOUT= PortUnusedBitMask;
	
	/* Enable outputs */
	PortDIR = _BV(NearLedBit) | _BV(FarLedBit) | _BV(Duet3K0OutputBit) | _BV(Duet3K6OutputBit);

	clearAll();

	/* Set up timer 1 in mode 2 (CTC mode) */
	GTCCR = 0;
	TCCR0A = _BV(WGM01); /* no direct outputs, mode 2, set the mode, clock stopped for now */
	TCCR0B = 0;

	TCNT0 = 0;
	RegTI = _BV(OCF0A); /* clear any pending interrupt */
	RegTM = _BV(OCIE0A); /* enable the timer 0 compare match B interrupt */
	OCR0A = baseTopIR;
	TCCR0B |= _BV(CS01); /* start the clock, prescaler = 8 */
	
	ADMUX = (uint8_t)AdcPhototransistorChan | (REF1V1);
	ADCSRA = _BV(ADEN) | _BV(ADATE) | ADC_PS; /* enable ADC, auto trigger enable, ADC clock ~= 125kHz */
	ADCSRB = _BV(ADTS1) | _BV(ADTS0); /* start conversion on timer 0 compare match A, unipolar input mode */

	tickCounter = 0;
	interrupts();

	check_mode();

	/* start collecting the data */
	clearAll();
	running = true;
	DelayTicks(4u * cyclesAveragedIR + 2);
}

int 
every100()
{
	static uint16_t x= 0;
	const uint16_t t = GetTicks();
	if(t - x  > 1000){
		x = t;
		return 1;
	}
	return 0;
}

void
loop()
{
	KickWatchdog();

	noInterrupts();
	const uint16_t locNearSum = nearData.sum;
	const uint16_t locFarSum = farData.sum;
	const uint16_t locOffSum = offData.sum;
	interrupts();

/*
	if (every100()) {
		Serial.print(locOffSum); Serial.print(' ');
		Serial.print(locNearSum); Serial.print(' ');
		Serial.println(locFarSum);
	}
*/

	const bool highSense = IS_HIGHSENSE(ADMUX);
	if(highSense && (locNearSum >= rangeUpThr || locFarSum >= rangeUpThr)){
		/* switch to low sensitivity (voltage reference is VCC) */
		ADMUX = (uint8_t)AdcPhototransistorChan | (REFVCC);
                RANGE_DEBUG;
		DelayTicks(4 * cyclesAveragedIR + 3);
		return;
	}
	else if(!highSense && locNearSum < rangeDownThr && locFarSum < rangeDownThr){
		/* switch to high sensitivity (voltage reference is 1.1V) */
		ADMUX = (uint8_t)AdcPhototransistorChan | (REF1V1);
                RANGE_DEBUG;
		DelayTicks(4 * cyclesAveragedIR + 3);
		return;
	}

	/*
          We only report saturation when both readings are in the saturation zone.
	  This allows for the possibility that the light from the far LED exceeds the saturation limit at large distances, but is in
	  range at the trigger height. Some sensors have been found to exhibit this behaviour with a target of plain glass when run from 5V.
	  We flash the LED rapidly to indicate this
	 */
	if(locFarSum >= saturatedThr && locNearSum > saturatedThr){
		if(GetTicks() & 512)
			SetOutputOff();
		else
			SetOutputSaturated();
		return;
	}

	const uint32_t adjNearSum = (uint32_t)delta(locNearSum, locOffSum) * nvData[0];
	const uint16_t hadjFarSum = delta(locFarSum, locOffSum);
	const uint32_t adjFarSum  = (uint32_t)hadjFarSum * nvData[1];

	const bool readingsOk = hadjFarSum >= farThr;
			
	if(readingsOk && adjNearSum >= adjFarSum){
		if(digitalOutput)
  			SetOutputSaturated();
		else
			SetOutputOn();
	}
	else if(!digitalOutput && readingsOk && adjNearSum + adjNearSum >= adjFarSum)
		SetOutputApproaching();
	else
		SetOutputOff();
}



