/*
    This project is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Deviation is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Arduino.h>
#include "nrf24l01.h"
#include "xn297_emu.h"


///////////////
// XN297 emulation layer
uint8_t xn297_scramble_enabled=XN297_SCRAMBLED;	//enabled by default
uint8_t xn297_addr_len;
uint8_t xn297_tx_addr[5];
uint8_t xn297_rx_addr[5];
uint8_t xn297_crc = 0;

// xn297 address / pcf / payload scramble table
const uint8_t xn297_scramble[] = {
    0xE3, 0xB1, 0x4B, 0xEA, 0x85, 0xBC, 0xE5, 0x66,
    0x0D, 0xAE, 0x8C, 0x88, 0x12, 0x69, 0xEE, 0x1F,
    0xC7, 0x62, 0x97, 0xD5, 0x0B, 0x79, 0xCA, 0xCC,
    0x1B, 0x5D, 0x19, 0x10, 0x24, 0xD3, 0xDC, 0x3F,
    0x8E, 0xC5, 0x2F, 0xAA, 0x16, 0xF3, 0x95 };

// scrambled, standard mode crc xorout table
const uint16_t PROGMEM xn297_crc_xorout_scrambled[] = {
    0x0000, 0x3448, 0x9BA7, 0x8BBB, 0x85E1, 0x3E8C,
    0x451E, 0x18E6, 0x6B24, 0xE7AB, 0x3828, 0x814B,
    0xD461, 0xF494, 0x2503, 0x691D, 0xFE8B, 0x9BA7,
    0x8B17, 0x2920, 0x8B5F, 0x61B1, 0xD391, 0x7401,
    0x2138, 0x129F, 0xB3A0, 0x2988, 0x23CA, 0xC0CB,
    0x0C6C, 0xB329, 0xA0A1, 0x0A16, 0xA9D0 };

// unscrambled, standard mode crc xorout table
const uint16_t PROGMEM xn297_crc_xorout[] = {
    0x0000, 0x3D5F, 0xA6F1, 0x3A23, 0xAA16, 0x1CAF,
    0x62B2, 0xE0EB, 0x0821, 0xBE07, 0x5F1A, 0xAF15,
    0x4F0A, 0xAD24, 0x5E48, 0xED34, 0x068C, 0xF2C9,
    0x1852, 0xDF36, 0x129D, 0xB17C, 0xD5F5, 0x70D7,
    0xB798, 0x5133, 0x67DB, 0xD94E, 0x0A5B, 0xE445,
    0xE6A5, 0x26E7, 0xBDAB, 0xC379, 0x8E20 };

// scrambled enhanced mode crc xorout table
const uint16_t PROGMEM xn297_crc_xorout_scrambled_enhanced[] = {
    0x0000, 0x7EBF, 0x3ECE, 0x07A4, 0xCA52, 0x343B,
    0x53F8, 0x8CD0, 0x9EAC, 0xD0C0, 0x150D, 0x5186,
    0xD251, 0xA46F, 0x8435, 0xFA2E, 0x7EBD, 0x3C7D,
    0x94E0, 0x3D5F, 0xA685, 0x4E47, 0xF045, 0xB483,
    0x7A1F, 0xDEA2, 0x9642, 0xBF4B, 0x032F, 0x01D2,
    0xDC86, 0x92A5, 0x183A, 0xB760, 0xA953 };

// unscrambled enhanced mode crc xorout table
// unused so far
#ifdef XN297DUMP_NRF24L01_INO
const uint16_t xn297_crc_xorout_enhanced[] = {
    0x0000, 0x8BE6, 0xD8EC, 0xB87A, 0x42DC, 0xAA89,
    0x83AF, 0x10E4, 0xE83E, 0x5C29, 0xAC76, 0x1C69,
    0xA4B2, 0x5961, 0xB4D3, 0x2A50, 0xCB27, 0x5128,
    0x7CDB, 0x7A14, 0xD5D2, 0x57D7, 0xE31D, 0xCE42,
    0x648D, 0xBF2D, 0x653B, 0x190C, 0x9117, 0x9A97,
    0xABFC, 0xE68E, 0x0DE7, 0x28A2, 0x1965 };
#endif

static uint8_t bit_reverse(uint8_t b_in)
{
    uint8_t b_out = 0;
    for (uint8_t i = 0; i < 8; ++i)
	{
        b_out = (b_out << 1) | (b_in & 1);
        b_in >>= 1;
    }
    return b_out;
}

static const uint16_t polynomial = 0x1021;
static uint16_t crc16_update(uint16_t crc, uint8_t a, uint8_t bits)
{
	crc ^= a << 8;
    while(bits--)
        if (crc & 0x8000)
            crc = (crc << 1) ^ polynomial;
		else
            crc = crc << 1;
    return crc;
}

void XN297_SetTXAddr(const uint8_t* addr, uint8_t len)
{
	if (len > 5) len = 5;
	if (len < 3) len = 3;
	uint8_t buf[] = { 0x55, 0x0F, 0x71, 0x0C, 0x00 }; // bytes for XN297 preamble 0xC710F55 (28 bit)
	xn297_addr_len = len;
	if (xn297_addr_len < 4)
		for (uint8_t i = 0; i < 4; ++i)
			buf[i] = buf[i+1];
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, len-2);
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, buf, 5);
	// Receive address is complicated. We need to use scrambled actual address as a receive address
	// but the TX code now assumes fixed 4-byte transmit address for preamble. We need to adjust it
	// first. Also, if the scrambled address begins with 1 nRF24 will look for preamble byte 0xAA
	// instead of 0x55 to ensure enough 0-1 transitions to tune the receiver. Still need to experiment
	// with receiving signals.
	memcpy(xn297_tx_addr, addr, len);
}

void XN297_SetRXAddr(const uint8_t* addr, uint8_t len)
{
	if (len > 5) len = 5;
	if (len < 3) len = 3;
	uint8_t buf[] = { 0, 0, 0, 0, 0 };
	memcpy(buf, addr, len);
	memcpy(xn297_rx_addr, addr, len);
	for (uint8_t i = 0; i < xn297_addr_len; ++i)
	{
		buf[i] = xn297_rx_addr[i];
		if(xn297_scramble_enabled)
			buf[i] ^= xn297_scramble[xn297_addr_len-i-1];
	}
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, len-2);
	NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, buf, 5);
}

void XN297_Configure(uint8_t flags)
{
	xn297_crc = !!(flags & _BV(NRF24L01_00_EN_CRC));
	flags &= ~(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO));
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, flags & 0xFF);
}

void XN297_SetScrambledMode(const uint8_t mode)
{
    xn297_scramble_enabled = mode;
}

void XN297_WritePayload(uint8_t* msg, uint8_t len)
{
	uint8_t buf[32];
	uint8_t last = 0;

	if (xn297_addr_len < 4)
	{
		// If address length (which is defined by receive address length)
		// is less than 4 the TX address can't fit the preamble, so the last
		// byte goes here
		buf[last++] = 0x55;
	}
	for (uint8_t i = 0; i < xn297_addr_len; ++i)
	{
		buf[last] = xn297_tx_addr[xn297_addr_len-i-1];
		if(xn297_scramble_enabled)
			buf[last] ^=  xn297_scramble[i];
		last++;
	}
	for (uint8_t i = 0; i < len; ++i)
	{
		// bit-reverse bytes in packet
		buf[last] = bit_reverse(msg[i]);
		if(xn297_scramble_enabled)
			buf[last] ^= xn297_scramble[xn297_addr_len+i];
		last++;
	}
	if (xn297_crc)
	{
		uint8_t offset = xn297_addr_len < 4 ? 1 : 0;
		uint16_t crc = 0xb5d2;
		for (uint8_t i = offset; i < last; ++i)
			crc = crc16_update(crc, buf[i], 8);
		if(xn297_scramble_enabled)
			crc ^= pgm_read_word(&xn297_crc_xorout_scrambled[xn297_addr_len - 3 + len]);
		else
			crc ^= pgm_read_word(&xn297_crc_xorout[xn297_addr_len - 3 + len]);
		buf[last++] = crc >> 8;
		buf[last++] = crc & 0xff;
	}
	NRF24L01_WritePayload(buf, last);
}

void XN297_WriteEnhancedPayload(uint8_t* msg, uint8_t len, uint8_t noack)
{
	uint8_t packet[32];
	uint8_t scramble_index=0;
	uint8_t last = 0;
	static uint8_t pid=0;

	// address
	if (xn297_addr_len < 4)
	{
		// If address length (which is defined by receive address length)
		// is less than 4 the TX address can't fit the preamble, so the last
		// byte goes here
		packet[last++] = 0x55;
	}
	for (uint8_t i = 0; i < xn297_addr_len; ++i)
	{
		packet[last] = xn297_tx_addr[xn297_addr_len-i-1];
		if(xn297_scramble_enabled)
			packet[last] ^= xn297_scramble[scramble_index++];
		last++;
	}

	// pcf
	packet[last] = (len << 1) | (pid>>1);
	if(xn297_scramble_enabled)
		packet[last] ^= xn297_scramble[scramble_index++];
	last++;
	packet[last] = (pid << 7) | (noack << 6);

	// payload
	packet[last]|= bit_reverse(msg[0]) >> 2; // first 6 bit of payload
	if(xn297_scramble_enabled)
		packet[last] ^= xn297_scramble[scramble_index++];

	for (uint8_t i = 0; i < len-1; ++i)
	{
		last++;
		packet[last] = (bit_reverse(msg[i]) << 6) | (bit_reverse(msg[i+1]) >> 2);
		if(xn297_scramble_enabled)
			packet[last] ^= xn297_scramble[scramble_index++];
	}

	last++;
	packet[last] = bit_reverse(msg[len-1]) << 6; // last 2 bit of payload
	if(xn297_scramble_enabled)
		packet[last] ^= xn297_scramble[scramble_index++] & 0xc0;

	// crc
	if (xn297_crc)
	{
		uint8_t offset = xn297_addr_len < 4 ? 1 : 0;
		uint16_t crc = 0xb5d2;
		for (uint8_t i = offset; i < last; ++i)
			crc = crc16_update(crc, packet[i], 8);
		crc = crc16_update(crc, packet[last] & 0xc0, 2);
		if (xn297_scramble_enabled)
			crc ^= pgm_read_word(&xn297_crc_xorout_scrambled_enhanced[xn297_addr_len-3+len]);
		//else
		//	crc ^= pgm_read_word(&xn297_crc_xorout_enhanced[xn297_addr_len - 3 + len]);

		packet[last++] |= (crc >> 8) >> 2;
		packet[last++] = ((crc >> 8) << 6) | ((crc & 0xff) >> 2);
		packet[last++] = (crc & 0xff) << 6;
	}
	NRF24L01_WritePayload(packet, last);

	pid++;
	if(pid>3)
		pid=0;
}

boolean XN297_ReadPayload(uint8_t* msg, uint8_t len)
{ //!!! Don't forget if using CRC to do a +2 on any of the used NRF24L01_11_RX_PW_Px !!!
	uint8_t buf[32];
	if (xn297_crc)
		NRF24L01_ReadPayload(buf, len+2);	// Read payload + CRC 
	else
		NRF24L01_ReadPayload(buf, len);
	// Decode payload
	for(uint8_t i=0; i<len; i++)
	{
		uint8_t b_in=buf[i];
		if(xn297_scramble_enabled)
			b_in ^= xn297_scramble[i+xn297_addr_len];
		msg[i] = bit_reverse(b_in);
	}
	if (!xn297_crc)
		return true;	// No CRC so OK by default...

	// Calculate CRC
	uint16_t crc = 0xb5d2;
	//process address
	for (uint8_t i = 0; i < xn297_addr_len; ++i)
	{
		uint8_t b_in=xn297_rx_addr[xn297_addr_len-i-1];
		if(xn297_scramble_enabled)
			b_in ^=  xn297_scramble[i];
		crc = crc16_update(crc, b_in, 8);
	}
	//process payload
	for (uint8_t i = 0; i < len; ++i)
		crc = crc16_update(crc, buf[i], 8);
	//xorout
	if(xn297_scramble_enabled)
		crc ^= pgm_read_word(&xn297_crc_xorout_scrambled[xn297_addr_len - 3 + len]);
	else
		crc ^= pgm_read_word(&xn297_crc_xorout[xn297_addr_len - 3 + len]);
	//test
	if( (crc >> 8) == buf[len] && (crc & 0xff) == buf[len+1])
		return true;	// CRC  OK
	return false;		// CRC NOK
}

uint8_t XN297_ReadEnhancedPayload(uint8_t* msg, uint8_t len)
{ //!!! Don't forget do a +2 and if using CRC add +2 on any of the used NRF24L01_11_RX_PW_Px !!!
	uint8_t buffer[32];
	uint8_t pcf_size; // pcf payload size
	if (xn297_crc)
		NRF24L01_ReadPayload(buffer, len+4);	// Read pcf + payload + CRC 
	else
		NRF24L01_ReadPayload(buffer, len+2);	// Read pcf + payload
	pcf_size = buffer[0];
	if(xn297_scramble_enabled)
		pcf_size ^= xn297_scramble[xn297_addr_len];
	pcf_size = pcf_size >> 1;
	for(int i=0; i<len; i++)
	{
		msg[i] = bit_reverse((buffer[i+1] << 2) | (buffer[i+2] >> 6));
		if(xn297_scramble_enabled)
			msg[i] ^= bit_reverse((xn297_scramble[xn297_addr_len+i+1] << 2) | 
									(xn297_scramble[xn297_addr_len+i+2] >> 6));
	}

	if (!xn297_crc)
		return pcf_size;	// No CRC so OK by default...

	// Calculate CRC
	uint16_t crc = 0xb5d2;
	//process address
	for (uint8_t i = 0; i < xn297_addr_len; ++i)
	{
		uint8_t b_in=xn297_rx_addr[xn297_addr_len-i-1];
		if(xn297_scramble_enabled)
			b_in ^=  xn297_scramble[i];
		crc = crc16_update(crc, b_in, 8);
	}
	//process payload
	for (uint8_t i = 0; i < len+1; ++i)
		crc = crc16_update(crc, buffer[i], 8);
	crc = crc16_update(crc, buffer[len+1] & 0xc0, 2);
	//xorout
	if (xn297_scramble_enabled)
		crc ^= pgm_read_word(&xn297_crc_xorout_scrambled_enhanced[xn297_addr_len-3+len]);
#ifdef XN297DUMP_NRF24L01_INO
	else
		crc ^= pgm_read_word(&xn297_crc_xorout_enhanced[xn297_addr_len - 3 + len]);
#endif
	uint16_t crcxored=(buffer[len+1]<<10)|(buffer[len+2]<<2)|(buffer[len+3]>>6) ;
	if( crc == crcxored)
		return pcf_size;	// CRC  OK
	return 0;				// CRC NOK
}
 
 // End of XN297 emulation
