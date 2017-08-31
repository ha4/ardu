/*
Copyright (c) 2007, Jim Studt

Updated to work with arduino-0008 and to include skip() as of
2007/07/06. --RJL20

Modified to calculate the 8-bit CRC directly, avoiding the need for
the 256-byte lookup table to be loaded in RAM.  Tested in arduino-0010
-- Tom Pollard, Jan 23, 2008

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Much of the code was inspired by Derek Yerger's code, though I don't
think much of that remains.  In any event that was..
    (copyleft) 2006 by Derek Yerger - Free to distribute freely.

The CRC code was excerpted and inspired by the Dallas Semiconductor 
sample code bearing this copyright.
//---------------------------------------------------------------------------
// Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
// OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// Except as contained in this notice, the name of Dallas Semiconductor
// shall not be used except as stated in the Dallas Semiconductor
// Branding Policy.
//--------------------------------------------------------------------------
*/

#include "OneWire.h"

OneWire::OneWire( uint8_t pinArg)
{
    pin = pinArg;
    start();
}

//
// Perform the onewire reset function.  We will wait up to 250uS for
// the bus to come high, if it doesn't then it is broken or shorted
// and we return a 0;
//
// Returns 1 if a device asserted a presence pulse, 0 otherwise.
//
uint8_t OneWire::reset() {
    uint8_t r;
    uint8_t retries = 125;

    // wait until the wire is high... just in case
    pinMode(pin,INPUT);
    do {
	if ( retries-- == 0) return 0;
	delayMicroseconds(2); 
    } while( !digitalRead( pin));
    
    digitalWrite(pin,0);   // pull low for 500uS
    pinMode(pin,OUTPUT);
    delayMicroseconds(500);

    pinMode(pin,INPUT);
    delayMicroseconds(65);

    r = !digitalRead(pin);
    delayMicroseconds(490);
    return r;
}

void OneWire::write_bit(uint8_t v) {

    pinMode(pin,OUTPUT);
    digitalWrite(pin,0);
    delayMicroseconds((v&1)?5:55);

    digitalWrite(pin,1);
    pinMode(pin,INPUT);
    delayMicroseconds((v&1)?55:5);
}

uint8_t OneWire::read_bit() {
    uint8_t r;
    
    pinMode(pin,OUTPUT);
    digitalWrite(pin,0);
    delayMicroseconds(1);

    digitalWrite(pin,1);
    pinMode(pin,INPUT);
    delayMicroseconds(5);          // A "read slot" is when 1mcs > t > 2mcs
    r = digitalRead(pin);

    delayMicroseconds(50);        // whole bit slot is 60-120uS, need to give some time
    
    return r;
}

void OneWire::power(bool p)
{
    digitalWrite(pin,1);
    if (p)
	pinMode(pin,OUTPUT);
    else
	pinMode(pin,INPUT);
}

void OneWire::write(uint8_t v) {
    uint8_t n;
    
    for (n=8; n--; v>>=1)
	write_bit(v);
}

uint8_t OneWire::read() {
    uint8_t n;
    uint8_t r = 0;
    
    for (r=0, n=8; n--; )
	r = (r>>1) | (read_bit() ? 128 : 0);

    return r;
}

void OneWire::vwrite(uint8_t *d, uint8_t n)
{
    uint8_t i;

    for (i=0; i < n; i++)   write(d[i]);
}

void OneWire::vread(uint8_t *d, uint8_t n)
{
    uint8_t i;

    for (i=0; i < n; i++)  d[i] = read();
}

void OneWire::start()
{
    uint8_t i;
    
    srchJ = -1;
    srchE = 0;
    for( i = 8; i--;) adr[i] = 0;
}

//
// Perform a search. If this function returns a '1' then it has
// enumerated the next device and you may retrieve the ROM from the
// OneWire::address variable. If there are no devices, no further
// devices, or something horrible happens in the middle of the
// enumeration then a 0 is returned.  If a new device is found then
// its address is copied to newAddr.  Use OneWire::reset_search() to
// start over.
// 
uint8_t OneWire::discover(uint8_t *newAddr)
{
    uint8_t i;
    char lastJunction = -1;
    uint8_t done = 1;
    
    if (srchE) return 0;
    for(i = 0; i < 64; i++) {
	uint8_t a = read_bit();
	uint8_t nota = read_bit();
	uint8_t ibyte = i/8;
	uint8_t ibit = 1<<(i&7);
	
        // This should not happen, this means nothing responded, but maybe if
        // something vanishes during the search it will come up.
	if (a && nota) return 0;

	if ( !a && !nota) {
	    if ( i == srchJ) {   // this is our time to decide differently, we went zero last time, go one.
		a = 1;
		srchJ = lastJunction;
	    } else if ( i < srchJ) {   // take whatever we took last time, look in address
		if (adr[ibyte]&ibit)
			a = 1;
		else { // Only 0s count as pending junctions, we've already exhasuted the 0 side of 1s
		    a = 0;
		    done = 0;
		    lastJunction = i;
		}
	    } else {   // we are blazing new tree, take the 0
		a = 0;
		srchJ = i;
		done = 0;
	    }
	    lastJunction = i;
	}

	if (a) adr[ibyte] |= ibit;
	else adr[ibyte] &= ~ibit;
	
	write_bit(a);
    }

    if (done) srchE = 1;
    for (i=8; i--;) newAddr[i]=adr[i];

    return 1;  
}

uint8_t OneWire::crc8( uint8_t *addr, uint8_t len)
{
    uint8_t i;
    uint8_t crc = 0;
    
    for (i = 0; i < len; i++) {
        uint8_t inbyte = addr[i];

        crc = (crc >> 1) ^ (((crc ^ inbyte) & 0x01) ? 0x8C : 0);  inbyte >>= 1;
        crc = (crc >> 1) ^ (((crc ^ inbyte) & 0x01) ? 0x8C : 0);  inbyte >>= 1;
        crc = (crc >> 1) ^ (((crc ^ inbyte) & 0x01) ? 0x8C : 0);  inbyte >>= 1;
        crc = (crc >> 1) ^ (((crc ^ inbyte) & 0x01) ? 0x8C : 0);  inbyte >>= 1;

        crc = (crc >> 1) ^ (((crc ^ inbyte) & 0x01) ? 0x8C : 0);  inbyte >>= 1;
        crc = (crc >> 1) ^ (((crc ^ inbyte) & 0x01) ? 0x8C : 0);  inbyte >>= 1;
        crc = (crc >> 1) ^ (((crc ^ inbyte) & 0x01) ? 0x8C : 0);  inbyte >>= 1;
        crc = (crc >> 1) ^ (((crc ^ inbyte) & 0x01) ? 0x8C : 0);  inbyte >>= 1;
    }
    return crc;
}
