   
void ps2_init(void);

/* request to transmit a character when possible. Care should be
   taken to not call this routine in the middle of already transmitting
   a character.
	test ps2_txClear() before send any data
   */
void ps2_txSet(unsigned char d);

/* this routine gets the recently received character. How do you
   know you have a character? Because ps2_flags has PS2_RX_BYTE set.
   It could also have PS2_RX_BAD set, which means a bad character
   was recieved (e.g. bad parity).
	check ps2_rxAvailable() before call it. This routine
	resets the PS2_RX_BYTE flag. */
unsigned char ps2_rxGet(void);

bool ps2_txClear(void);
bool ps2_rxAvailable(void);

void ps2_process(void);
