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
    uint8_t port;
    datapin = pinArg;
    _bit = digitalPinToBitMask(datapin);
    port = digitalPinToPort(datapin);
    if (port == NOT_A_PIN) return;

    _reg = portModeRegister(port);
    _out = portOutputRegister(port);
    _pin = portInputRegister(port);

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
    *_reg &= ~_bit; // input
    *_out &= ~_bit; // no pullup

    do {
	if ( retries-- == 0) return 0;
	delayMicroseconds(2);  // 250 us wait
    } while(!(*_pin & _bit));
    
    *_reg |= _bit;   // pull low for 500uS
    delayMicroseconds(500);

    *_reg &= ~_bit;
    delayMicroseconds(65);

    r = !(*_pin & _bit);
    delayMicroseconds(490);
    return r;
}

void OneWire::write_bit(uint8_t v) {

    *_out &= ~_bit; // no pullup
    *_reg |= _bit; // output low
    delayMicroseconds((v&1)?5:55);

    *_reg &= ~_bit; // output release
    delayMicroseconds((v&1)?55:5);
}

uint8_t OneWire::read_bit() {
    uint8_t r;
    
    *_out &= ~_bit; // no pullup
    *_reg |= _bit; // output low
    delayMicroseconds(1);

    *_reg &= ~_bit; // output release
    delayMicroseconds(5);          // A "read slot" is when 1mcs > t > 2mcs
    r = (*_pin & _bit)!=0;

    delayMicroseconds(50);        // whole bit slot is 60-120uS, need to give some time
    
    return r;
}

void OneWire::power(bool p)
{
    if (p) {
        *_out |= _bit; // force 1
        *_reg |= _bit; // output high
    } else {
        *_out &= ~_bit; // 0
        *_reg &= ~_bit; // release
    }
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

void OneWire::write(uint8_t *d, uint8_t n)
{
    uint8_t i;

    for (i=0; i < n; i++)   write(d[i]);
}

void OneWire::read(uint8_t *d, uint8_t n)
{
    uint8_t i;

    for (i=0; i < n; i++)  d[i] = read();
}

void OneWire::start()
{
    srchJ = -1;
    srchE = 0;
    for(int i = 8; i--;) adr[i] = 0x00;
}

uint8_t OneWire::discover(uint8_t *newAddr)
{
   int8_t lastZJ = -1;

   if (srchE) return 0;

   for(int i = 0; i < 64; i++)  {
	char msk = 1<<(i&7), ptr = i>>3;
	uint8_t  a = read_bit();
	uint8_t _a = read_bit();

	if (a && _a ) return 0;
	if (a == _a) {
                if (i==srchJ) a=1;   // if (i < srchJ) a = ((adr[ptr]&msk) != 0)?1:0;
	   else if(adr[ptr]&msk) a=1;// else           a = (i==srchJ)?1:0;
	   else lastZJ = i;          // if (a == 0) lastZJ = i;
	}

	if(a) adr[ptr] |=  msk;
	else  adr[ptr] &= ~msk;
    	write_bit(a);
   }

   srchJ = lastZJ;
   if (lastZJ == -1) srchE = 1;
   
   for (int m = 8; m--;) newAddr[m] = adr[m];
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

uint8_t OneWire::search(uint8_t *newAddr)
{
    if (!reset()) return 0;
    write(0xF0); // search rom
    if (discover(newAddr)) return 1;
    start(); // no devices, start over
    return 0;
}

void OneWire::select(uint8_t *newAddr)
{
    if (!reset()) return;
    write(0x55);  // match rom
    write(newAddr, 8); 
}

