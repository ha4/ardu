#if defined(ARDUINO) && (ARDUINO >= 100)
#	include <Arduino.h>
#else
#	if !defined(IRPRONTO)
#		include <WProgram.h>
#	endif
#endif

#include <IRremote.h>


void irisr()
{
	// Read if IR Receiver -> SPACE [xmt LED off] or a MARK [xmt LED on]
	uint8_t irdata = digitalRead(irparams.recvpin);
	unsigned int t = micros();

	// irparams.timer++;  // One more 50uS tick
	if (irparams.rawlen >= RAWBUF)  irparams.rcvstate = STATE_OVERFLOW ;

	switch(irparams.rcvstate) {
	case STATE_IDLE: // In the middle of a gap
		if (irdata == MARK) {
			if ((t-irparams.timer) >= _GAP) { // Gap just ended; Record duration; Start recording transmission
				irparams.overflow = false;
				irparams.rawlen = 0;
				irparams.rawbuf[irparams.rawlen++] = (t-irparams.timer)/50;
				irparams.rcvstate= STATE_MARK;
        		}
			irparams.timer = t;
		}
		break;
	case STATE_MARK:
		if (irdata == SPACE) {   // Mark ended; Record time
			irparams.rawbuf[irparams.rawlen++] = (t-irparams.timer)/50;
			irparams.timer = t;
			irparams.rcvstate = STATE_SPACE;
		}
		break;

	case STATE_SPACE:  // Timing Space
		if (irdata == MARK) {  // Space just ended; Record time
			irparams.rawbuf[irparams.rawlen++] = (t-irparams.timer)/50;
			irparams.timer                     = t;
			irparams.rcvstate                  = STATE_MARK;
                } else if ((t-irparams.timer) > _GAP) {  // Space
				// A long Space, indicates gap between codes
				// Flag the current code as ready for processing
				// Switch to STOP
				// Don't reset timer; keep counting Space width
				irparams.rcvstate = STATE_STOP;
		}
		break;
	case STATE_STOP:  // Waiting; Measuring Gap
		 if (irdata == MARK)  irparams.timer = t ;  // Reset gap timer
		 break;
	case STATE_OVERFLOW:  // Flag up a read overflow; Stop the State Machine
		irparams.overflow = true;
		irparams.rcvstate = STATE_STOP;
		break;
	}
}

void resume_irrecv()
{
	irparams.rcvstate = STATE_IDLE;
	irparams.rawlen = 0;
}

int  getRClevel (decode_results *results,  int *offset,  int *used,  int t1)
{
	int  width;
	int  val;
	int  correction;
	int  avail;

	if (*offset >= results->rawlen)  return SPACE ;  // After end of recorded buffer, assume SPACE.
	width      = results->rawbuf[*offset];
	val        = ((*offset) % 2) ? MARK : SPACE;
	correction = (val == MARK) ? MARK_EXCESS : - MARK_EXCESS;

	if      (MATCH(width, (  t1) + correction))  avail = 1 ;
	else if (MATCH(width, (2*t1) + correction))  avail = 2 ;
	else if (MATCH(width, (3*t1) + correction))  avail = 3 ;
	else                                         return -1 ;

	(*used)++;
	if (*used >= avail) {
		*used = 0;
		(*offset)++;
	}

	return val;
}

#define MIN_RC5_SAMPLES     11
#define RC5_T1             889
#define RC5_RPT_LENGTH   46000

bool  decodeRC5_irrecv (decode_results *results)
{
	int   nbits;
	long  data   = 0;
	int   used   = 0;
	int   offset = 1;  // Skip gap space

	if (irparams.rawlen < MIN_RC5_SAMPLES + 2)  return false ;

	// Get start bits
	if (getRClevel(results, &offset, &used, RC5_T1) != MARK)   return false ;
	if (getRClevel(results, &offset, &used, RC5_T1) != SPACE)  return false ;
	if (getRClevel(results, &offset, &used, RC5_T1) != MARK)   return false ;

	for (nbits = 0;  offset < irparams.rawlen;  nbits++) {
		int  levelA = getRClevel(results, &offset, &used, RC5_T1);
		int  levelB = getRClevel(results, &offset, &used, RC5_T1);

		if      ((levelA == SPACE) && (levelB == MARK ))  data = (data << 1) | 1 ;
		else if ((levelA == MARK ) && (levelB == SPACE))  data = (data << 1) | 0 ;
		else                                              return false ;
	}

	// Success
	results->bits        = nbits;
	results->value       = data;
	results->decode_type = RC5;
	return true;
}

void enable_irrecv(int pin)
{
	irparams.recvpin = pin;
	irparams.blinkflag = 0;
	irparams.rcvstate = STATE_IDLE;
	irparams.rawlen = 0;
        irparams.timer = micros();
        pinMode(pin, INPUT_PULLUP);
}

