#define SUPPORT_CALIBRATION	(0)
// set nonzero to flash LED when it changes range
#define RANGE_DEBUG			(0)

	
// Pin assignments tiny25:
// PB0/MOSI			far LED drive, active high
// PB1/MISO			near LED drive, active high
// PB2/ADC1/SCK		output to Duet via 12K resistor
// PB3/ADC3			output to Duet via 10K resistor
// PB4/ADC2			input from phototransistor
// PB5/ADC0/RESET	not available, used for programming

#ifndef __ECV__
/* fuse for attiny */
//__fuse_t __fuse __attribute__((section (".fuse"))) = {0xE2u, 0xDFu, 0xFFu};
#endif

#define PortOUT PORTD
#define PortDIR DDRD

#define REFVCC 0b01000000
#define REF1V1 0b11000000

// active - high
//#define PortOn(b) PortOUT |= _BV(b)
//#define PortOff(b) PortOUT &= ~_BV(b)
// active - low
#define PortOn(b) PortOUT &= ~_BV(b)
#define PortOff(b) PortOUT |= _BV(b)


const unsigned int AdcPhototransistorChan = 0;	// A0, D14, PC0 ADC channel for the phototransistor
const unsigned int AdcDuet3K0OutputChan = 2;// ADC channel for the 3K0 and LED output bit, when we use it as an input
const unsigned int AdcDuet3K6OutputChan = 1;// ADC channel for the 3K6 output bit, when we use it as an input
const unsigned int NearLedBit = 4;
const unsigned int FarLedBit = 3;
const unsigned int Duet3K0OutputBit = 5;
const unsigned int Duet3K6OutputBit = 6;

const uint8_t PortUnusedBitMask = 0;

// Give a G31 reading of about 0
inline void SetOutputOff(){PortOUT &= ~_BV(Duet3K0OutputBit); PortOUT &= ~_BV(Duet3K6OutputBit); digitalWrite(13,0); }
// Give a G31 reading of about 445 indicating we are approaching the trigger point
inline void SetOutputApproaching(){ PortOUT &= ~_BV(Duet3K0OutputBit); PortOUT |= _BV(Duet3K6OutputBit); digitalWrite(13,0); }
// Give a G31 reading of about 578 indicating we are at/past the trigger point
inline void SetOutputOn(){ PortOUT |= _BV(Duet3K0OutputBit); PortOUT &= ~_BV(Duet3K6OutputBit); digitalWrite(13,1); }
// Give a G31 reading of about 1023 indicating that the sensor is saturating
inline void SetOutputSaturated(){ PortOUT |= _BV(Duet3K0OutputBit); PortOUT |= _BV(Duet3K6OutputBit); digitalWrite(13,1); }

// IR parameters. These also allow us to receive a signal through the command input.
const uint16_t interruptFreq = 8000;						// interrupt frequency. We run the IR sensor at one quarter of this, i.e. 2kHz
															// highest usable value is about 9.25kHz because the ADC needs 13.5 clock cycles per conversion.
const uint16_t divisorIR = (uint16_t)(F_CPU/interruptFreq);
const uint16_t prescalerIR = 8;
const uint16_t baseTopIR = (divisorIR/prescalerIR) - 1u;
const uint16_t cyclesAveragedIR = 8;

const uint16_t farThreshold = 10u * cyclesAveragedIR;		// minimum far reading for us to think the sensor is working properly
const uint16_t saturatedThreshold = 870u * cyclesAveragedIR; // minimum reading for which we consider the sensor saturated
const uint16_t rangeUpThreshold = 700u;						// must be less than saturatedThreshold
const uint16_t rangeDownThreshold = 120u;					// must be less than about 1/5 rangeUpThreshold

const uint16_t ledFlashTime = interruptFreq/6u;				// how long the LED is on or off for when we are flashing it

// IR variables
typedef uint8_t irIndex_t;

class IrData
{
public:
	uint16_t readings[cyclesAveragedIR];
	volatile uint16_t sum;
	irIndex_t index;

	void addReading(uint16_t arg)
	{
		sum = sum - readings[index] + arg;
		readings[index] = arg;
		index = static_cast<irIndex_t>((index + 1u) % cyclesAveragedIR);
	}
	
        void init();
	IrData() : index(0) {}
};

// Initialize the IR data structure
void IrData::init()
{
	for (uint8_t i = 0; i < cyclesAveragedIR; ++i)
		readings[i] = 0;
	index = 0;
	sum = 0;
}

IrData nearData, farData, offData;
	
// General variables
volatile uint16_t tickCounter = 0;					// counts system ticks, lower 2 bits also used for ADC/LED state

bool digitalOutput = false;
volatile bool running = false;						// volatile because we care about when it is written

#if SUPPORT_CALIBRATION

const uint16_t eepromMagic = 0x52A6;

struct NvData
{
	uint16_t nearMultiplier;
	uint16_t farMultiplier;
	uint16_t checksum;
} nvData;

// EEPROM access functions
void readEEPROM(uint8_t ucAddress, uint8_t *p, uint8_t len)
{
	do {
		/* Wait for completion of previous write */
		while(EECR & (1<<EEPE));
		EEAR = ucAddress++;
		EECR |= (1<<EERE);
		*p++ = EEDR;
	} while (--len != 0);
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
	} while (--len != 0);
}

#endif

// ISR for the timer 0 compare match interrupt
// This uses 19 bytes of stack (from assembly listing, 2016-07-30)

ISR(TIMER0_COMPA_vect)
{
//  while((ADCSRA & ADIF) == 0);
	const uint16_t adcVal = ADC & 0x3FF;
	const uint8_t locTickCounter = (uint8_t)tickCounter;
	while (TCNT0 < 3u * 8u) {}						// delay a little until the ADC s/h has taken effect. 3 ADC clocks should be enough, and 1 ADC clock is 8 timer 0 clocks.

	switch(locTickCounter & 0x03u) {
		case 0: // Far LED is on, we just did no reading, we are doing a far reading now and an off reading next
			PortOff(FarLedBit);		// turn far LED off
			break;
		
		case 1: // LEDs are off, we just did a far reading, we are doing a off reading now and a near reading next			
			if (running)
				farData.addReading(adcVal);
			PortOn(NearLedBit);		// turn near LED on
			break;
					
		case 2: // Near LED is on, we just did an off reading, we are doing a near reading now and a dummy off reading next
			if (running)
				offData.addReading(adcVal);
			PortOff(NearLedBit);		// turn near LED off
			break;

		case 3: // Far LED is on, we just did an off reading, we are doing another off reading now which will be discarded
			if (running)
				nearData.addReading(adcVal);
			PortOn(FarLedBit);		// turn far LED on
			break;
	}
++tickCounter;
}


// Get the tick counter from outside the ISR. As it's more than 8 bits long, we need to disable interrupts while fetching it.
inline uint16_t GetTicks()
{
	noInterrupts();
	uint16_t ticks = tickCounter;
	interrupts();
	return ticks;
}

// Check whether we need to kick the watchdog
inline void KickWatchdog()
{
}

// Delay for a specified number of ticks, keeping the watchdog happy
void DelayTicks(uint16_t ticks)
{
	const uint16_t startTicks = GetTicks();
	do 
	{
		KickWatchdog();
	} while (GetTicks() - startTicks < ticks);
}


// Main program
void setup(void)
{
  	uint8_t flashesToGo;

	noInterrupts();
//	DIDR0 = _BV(AdcPhototransistorChan) | _BV(PortBDuet3K6OutputBit);// disable digital input buffers on ADC inputs

	// Set ports and pullup resistors
	PortOUT= PortUnusedBitMask;			// enable pullup on unused I/O pins
	
	// Enable outputs
	PortDIR = _BV(NearLedBit) | _BV(FarLedBit) | _BV(Duet3K0OutputBit) | _BV(Duet3K6OutputBit);
        digitalWrite(13,0);
        pinMode(13,OUTPUT);
	interrupts();

	running = false;
	nearData.init();
	farData.init();
	offData.init();

        Serial.begin(115200);

	noInterrupts();
	// Set up timer 1 in mode 2 (CTC mode)
	GTCCR = 0;
	TCCR0A = _BV(WGM01);			// no direct outputs, mode 2
	TCCR0B = 0;				// set the mode, clock stopped for now
	TCNT0 = 0;
	OCR0A = baseTopIR;
	OCR0B = 0;
	TIFR0 = _BV(OCF0A);			// clear any pending interrupt
	TIMSK0 = _BV(OCIE0A);			// enable the timer 0 compare match B interrupt
	TCCR0B |= _BV(CS01);			// start the clock, prescaler = 8
	
	ADMUX = (uint8_t)AdcDuet3K6OutputChan | (REFVCC);// select the 10K resistor output bit, single ended mode
	ADCSRA = _BV(ADEN) | _BV(ADATE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);// enable ADC, auto trigger enable, prescaler = 64 (ADC clock ~= 125kHz)
	ADCSRB = _BV(ADTS1) | _BV(ADTS0);	// start conversion on timer 0 compare match B, unipolar input mode
	tickCounter = 0;
	interrupts();

	
	// Determine whether to provide a digital output or a 4-state output, or to calibrate the sensor.
	// We do this by checking to see whether the connected electronics provided a pullup resistor on the output.
	// If a pullup resistor is detected, we provide a digital output, else we provide an analog output.
	// Wait a while before we do this test, so that Duet firmware has a chance to turn the internal pullup (50K to 150K) off,
	// and Arduino/RAMPS firmware has a chance to turn the internal pullup (20K to 50K) on.
	SetOutputOff();
	PortDIR &= ~_BV(Duet3K6OutputBit);	// set the pin to an input, pullup disabled because output is off
	
	DelayTicks(4u);
	running = true;	// start collecting readings

#if SUPPORT_CALIBRATION
	// Wait 1 second to allow voltages to stabilize
	DelayTicks(1u * interruptFreq);
	noInterrupts();
	uint16_t totalSum = offData.sum + nearData.sum + farData.sum;
	interrupts();
	bool inProgrammingSeq = totalSum >= cyclesAveragedIR * 950u * 3u;// see if output high after 1 second
	DelayTicks(2u * interruptFreq);
	if (inProgrammingSeq)
	{
		noInterrupts();
		totalSum = offData.sum + nearData.sum + farData.sum;
		interrupts();
		inProgrammingSeq = totalSum <= cyclesAveragedIR * 50u * 3u;// see if output low after 3 seconds
	}
	DelayTicks(2u * interruptFreq);
#else
	DelayTicks(5u * interruptFreq);		// wait 5 seconds
#endif
	running = false;			// stop collecting readings
	
	// Change back to normal operation mode
	ADMUX = (uint8_t)AdcPhototransistorChan | (REF1V1);// select ADC input from phototransistor, single ended mode, 1.1V reference
	PortDIR |= _BV(Duet3K6OutputBit);	// set the pin back to being an output

	// Readings have been collected into all three of nearData, farData, and offData.
	// We are looking for a pullup resistor of no more than 160K on the output to indicate that we should use a digital output.
	
#if SUPPORT_CALIBRATION
	totalSum = offData.sum + nearData.sum + farData.sum;

	if (inProgrammingSeq && totalSum >= cyclesAveragedIR * 900u * 3u)
	{
		// Output pin has been shorted to Vcc at the correct times, so enter calibration mode
		// Clear out the data and start collecting data from the phototransistor
		nearData.init();
		farData.init();
		offData.init();
		
		running = true;				// tell interrupt handler to collect readings
		DelayTicks(8u * cyclesAveragedIR + 4);	// wait until we have taken two full sets of readings
		running = false;
		
		// Get local copies of volatile variables to save code space. No need to disable interrupts because running is false.
		uint16_t locNearSum = nearData.sum;
		uint16_t locFarSum = farData.sum;
		uint16_t locOffSum = offData.sum;
		
		if (locNearSum >= rangeUpThreshold * cyclesAveragedIR || locFarSum >= rangeUpThreshold * cyclesAveragedIR)
		{
			ADMUX = (uint8_t)AdcPhototransistorChan | (REFVCC);		// switch to low sensitivity (voltage reference is VCC)
			running = true;
			DelayTicks(8u * cyclesAveragedIR + 4);
			running = false;
			locNearSum = nearData.sum;
			locFarSum = farData.sum;
			locOffSum = offData.sum;
		}

		const uint16_t nearSum = (locNearSum > locOffSum) ? locNearSum - locOffSum : 0;
		const uint16_t farSum = (locFarSum > locOffSum) ? locFarSum - locOffSum : 0;
		if (farSum >= farThreshold && locNearSum < saturatedThreshold  && locFarSum < saturatedThreshold  && nearSum <= 2 * farSum && farSum <= nearSum) {
			// the far LED is stronger than the near one, so to avoid false triggering we don't want the far reading to exceed the near one
			// Successful calibration so set multipliers and save to EEPROM
			nvData.nearMultiplier = farSum;
			nvData.farMultiplier = nearSum;
			nvData.checksum = nvData.nearMultiplier ^ nvData.farMultiplier ^ eepromMagic;
			writeEEPROM(0, reinterpret_cast<const uint8_t*>(&nvData), sizeof(nvData));
			
			flashesToGo = 6u;									// indicate successful calibration
		} else {
			SetOutputOn();										// light LED to signal calibration error
			for (;;) KickWatchdog();
		}
	} else {
		digitalOutput = totalSum >= (3000UL * cyclesAveragedIR * 1024UL * 3u)/(160000UL + 3000UL);
		flashesToGo = (digitalOutput) ? 2u : 4u;
		
		// Read multipliers from EEPROM
		readEEPROM(0u, reinterpret_cast<uint8_t*>(&nvData), sizeof(nvData));
		if ((nvData.nearMultiplier ^ nvData.farMultiplier ^ nvData.checksum) != eepromMagic)
		{
			nvData.nearMultiplier = nvData.farMultiplier = 1000;
			++flashesToGo;										// extra flash to indicate not calibrated
		}	
	}
#else
	const uint16_t totalSum = offData.sum + nearData.sum + farData.sum;
	digitalOutput = totalSum >= (3000UL * cyclesAveragedIR * 1024UL * 3u)/(160000UL + 3000UL);
	flashesToGo = (digitalOutput) ? 2u : 4u;
#endif
	
	// Flash the LED the appropriate number of times
	while (flashesToGo != 0u)
	{
		SetOutputSaturated();// turn LED on
		DelayTicks(ledFlashTime);
		SetOutputOff();	// turn LED off
		DelayTicks(ledFlashTime);
		--flashesToGo;
	}

	// start collecting the data
	nearData.init();
	farData.init();
	offData.init();
	running = true;
	DelayTicks(4u * cyclesAveragedIR + 2);
}

int every100() {
    static uint16_t x= 0;
    const uint16_t t = GetTicks();
    if (t - x  > 1000) { x = t; return 1; }
    return 0;
}

void loop()
{
	KickWatchdog();
	noInterrupts();
	const uint16_t locNearSum = nearData.sum;
	const uint16_t locFarSum = farData.sum;
	const uint16_t locOffSum = offData.sum;
	interrupts();

        if (every100()) {
          Serial.print(locOffSum); Serial.print(' ');
          Serial.print(locNearSum); Serial.print(' ');
          Serial.println(locFarSum);
        }

	// See if we need to switch the sensitivity range
	const bool highSense = (ADMUX & (1 << REFS1)) != 0;
	if (highSense && (locNearSum >= rangeUpThreshold * cyclesAveragedIR || locFarSum >= rangeUpThreshold * cyclesAveragedIR)) {
		ADMUX = (uint8_t)AdcPhototransistorChan | (REFVCC);	// switch to low sensitivity (voltage reference is VCC)
#if RANGE_DEBUG
		SetOutputOn();		// so we can see when it changes range
#endif
		DelayTicks(4 * cyclesAveragedIR + 3);
		return;
	} else if (!highSense && locNearSum < rangeDownThreshold * cyclesAveragedIR && locFarSum < rangeDownThreshold * cyclesAveragedIR) {
		ADMUX = (uint8_t)AdcPhototransistorChan | (REF1V1);	// switch to high sensitivity (voltage reference is 1.1V)
#if RANGE_DEBUG
		SetOutputOn();		// so we can see when it changes range
#endif
		DelayTicks(4 * cyclesAveragedIR + 3);
		return;
	}

	// We only report saturation when both readings are in the saturation zone.
	// This allows for the possibility that the light from the far LED exceeds the saturation limit at large distances, but is in
	// range at the trigger height. Some sensors have been found to exhibit this behaviour with a target of plain glass when run from 5V.
	const bool saturated =
#if SUPPORT_CALIBRATION
	(nvData.nearMultiplier >= nvData.farMultiplier)
		? (locFarSum >= saturatedThreshold && (uint32_t)locNearSum * nvData.nearMultiplier > (uint32_t)saturatedThreshold * nvData.farMultiplier)
  	      : (locNearSum >= saturatedThreshold && (uint32_t)locFarSum * nvData.farMultiplier > (uint32_t)saturatedThreshold * nvData.nearMultiplier);
#else
	locFarSum >= saturatedThreshold && locNearSum > saturatedThreshold;
#endif
	if (saturated) {
		// Sensor is saturating. We flash the LED rapidly to indicate this.
		if ((GetTicks() & (1 << 9)) != 0)
			SetOutputOff();
		else
			SetOutputSaturated();
		return;
	}

#if SUPPORT_CALIBRATION
	const uint32_t adjNearSum =  (locNearSum > locOffSum) ? (uint32_t)(locNearSum - locOffSum) * nvData.nearMultiplier : 0;
	const uint32_t adjFarSum = (locFarSum > locOffSum) ? (uint32_t)(locFarSum - locOffSum) * nvData.farMultiplier : 0;
#else
	const uint32_t adjNearSum =  (locNearSum > locOffSum) ? (uint32_t)(locNearSum - locOffSum) : 0;
	const uint32_t adjFarSum = (locFarSum > locOffSum) ? (uint32_t)(locFarSum - locOffSum) : 0;
#endif
	const bool readingsOk = locFarSum > locOffSum && locFarSum - locOffSum >= farThreshold;
			
	if (readingsOk && adjNearSum >= adjFarSum) {
		if (digitalOutput)
  			SetOutputSaturated();
		else
			SetOutputOn();
	} else if (!digitalOutput && readingsOk && adjNearSum * 2 >= adjFarSum) {
		SetOutputApproaching();
	} else {
		SetOutputOff();
	}
}



