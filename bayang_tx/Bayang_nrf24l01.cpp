/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

Multiprotocol is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Multiprotocol.  If not, see <http://www.gnu.org/licenses/>.
 */
// Compatible with EAchine H8 mini, H10, BayangToys X6/X7/X9, JJRC JJ850 ...
// Last sync with hexfet new_protocols/bayang_nrf24l01.c dated 2015-12-22

#include <Arduino.h>
#include "iface_nrf24l01.h"
#include "interface.h"

uint8_t protocol_flags;

#define BIND_IN_PROGRESS	protocol_flags &= ~_BV(7)
#define BIND_DONE			protocol_flags |= _BV(7)
#define IS_BIND_DONE		( ( protocol_flags & _BV(7) ) !=0 )
#define IS_BIND_IN_PROGRESS	( ( protocol_flags & _BV(7) ) ==0 )

#define BAYANG_BIND_COUNT		1000
#define BAYANG_PACKET_PERIOD	2000
#define BAYANG_PACKET_TELEM_PERIOD	5000
#define BAYANG_INITIAL_WAIT		500
#define BAYANG_PACKET_SIZE		15
#define BAYANG_RF_NUM_CHANNELS	4
#define BAYANG_RF_BIND_CHANNEL	0
#define BAYANG_RF_BIND_CHANNEL_X16_AH 10
#define BAYANG_ADDRESS_LENGTH	5

enum BAYANG_OPTION_FLAGS {
	BAYANG_OPTION_FLAG_TELEMETRY	= 0x01,
	BAYANG_OPTION_FLAG_ANALOGAUX	= 0x02,
};

uint8_t  rx_tx_addr[5];
uint8_t  rx_id[5];
uint8_t  phase;
uint8_t  hopping_frequency[BAYANG_RF_NUM_CHANNELS];
uint8_t  hopping_frequency_no=0;
uint8_t  rf_ch_num;
uint8_t  packet[15];
uint16_t bind_counter;
uint8_t  packet_count;


static void __attribute__((unused)) BAYANG_send_packet()
{
	uint8_t i;
	if (IS_BIND_IN_PROGRESS) {
		packet[0]= 0xA4;

		for(i=0;i<5;i++)
			packet[i+1]=rx_tx_addr[i];
		for(i=0;i<4;i++)
			packet[i+6]=hopping_frequency[i];
		packet[10] = rx_tx_addr[0];	// txid[0]
		packet[11] = rx_tx_addr[1];	// txid[1]
	} else {
		uint16_t val;
		packet[0] = 0xA5;
		packet[1] = 0xFA;		// normal mode is 0xF7, expert 0xFa , D4 normal is 0xF4
	}
	packet[12] = rx_tx_addr[2];	// txid[2]
	packet[13] = 0x0A;
	packet[14] = 0;
	for (uint8_t i=0; i < BAYANG_PACKET_SIZE-1; i++)
		packet[14] += packet[i];

	NRF24L01_WriteReg(NRF24L01_05_RF_CH, IS_BIND_IN_PROGRESS ? rf_ch_num:hopping_frequency[hopping_frequency_no++]);
	hopping_frequency_no%=BAYANG_RF_NUM_CHANNELS;

	// Power on, TX mode, 2byte CRC
	// Why CRC0? xn297 does not interpret it - either 16-bit CRC or nothing
	NRF24L01_FlushTx();
	NRF24L01_SetTxRxMode(TX_EN);
	XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
	XN297_WritePayload(packet, BAYANG_PACKET_SIZE);

//	NRF24L01_SetPower();	// Set tx_power
}

void  BAYANG_bildchannels(uint16_t *AETR1234val)
{
    uint16_t val;
    //Flags packet[2]
    packet[2] = 0x00;
      packet[2] = BAYANG_FLAG_FLIP;
      packet[2] |= BAYANG_FLAG_RTH;
      packet[2] |= BAYANG_FLAG_PICTURE;
      packet[2] |= BAYANG_FLAG_VIDEO;
      packet[2] |= BAYANG_FLAG_HEADLESS;
    //Flags packet[3]
    packet[3] = 0x00;
      packet[3] = BAYANG_FLAG_INVERTED;
      packet[3] |= BAYANG_FLAG_TAKE_OFF;
      packet[3] |= BAYANG_FLAG_EMG_STOP;
    //Aileron
    val = AETR1234val[0];
    packet[4] = (val>>8) + 0x7C;
    packet[5] = val & 0xFF;
    //Elevator
    val = AETR1234val[1];
    packet[6] = (val>>8) + 0x7C;
    packet[7] = val & 0xFF;
    //Throttle
    val = AETR1234val[2];
    packet[8] = (val>>8) + 0x7C;
    packet[9] = val & 0xFF;
    //Rudder
    val = AETR1234val[3];
    packet[10] = (val>>8) + 0x7C;
    packet[11] = val & 0xFF;

}

static void __attribute__((unused)) BAYANG_init()
{
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);

	XN297_SetTXAddr((uint8_t *)"\x00\x00\x00\x00\x00", BAYANG_ADDRESS_LENGTH);

	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     	// Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      	// No Auto Acknowldgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  	// Enable data pipe 0 only
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, BAYANG_PACKET_SIZE);
	NRF24L01_SetBitrate(NRF24L01_BR_1M);             	// 1Mbps
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00);	// No retransmits
	NRF24L01_SetPower(TXPOWER_100mW);
	NRF24L01_Activate(0x73);							// Activate feature register
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);			// Disable dynamic payload length on all pipes
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);
	NRF24L01_Activate(0x73);
	
	rf_ch_num = BAYANG_RF_BIND_CHANNEL;
}

enum {
	BAYANG_BIND=0,
	BAYANG_WRITE,
	BAYANG_CHECK,
	BAYANG_READ,
};

#define BAYANG_CHECK_DELAY		1000		// Time after write phase to check write complete
#define BAYANG_READ_DELAY		600			// Time before read phase

uint16_t BAYANG_callback(uint16_t *AETR1234)
{
	switch(phase)
	{
		case BAYANG_BIND:
			if (--bind_counter == 0)
			{
				XN297_SetTXAddr(rx_tx_addr, BAYANG_ADDRESS_LENGTH);
				BIND_DONE;
				phase++;	//WRITE
			}
			else
				BAYANG_send_packet();
			break;
		case BAYANG_WRITE:
      BAYANG_bildchannels(AETR1234);
			BAYANG_send_packet();
			break;
	}
	return BAYANG_PACKET_PERIOD;
}

static void __attribute__((unused)) BAYANG_initialize_txid()
{
	//Could be using txid[0..2] but using rx_tx_addr everywhere instead...
	hopping_frequency[0]=0;
	hopping_frequency[1]=(rx_tx_addr[3]&0x1F)+0x10;
	hopping_frequency[2]=hopping_frequency[1]+0x20;
	hopping_frequency[3]=hopping_frequency[2]+0x20;
	hopping_frequency_no=0;
}

uint16_t initBAYANG(void)
{
	BIND_IN_PROGRESS;	// autobind protocol
	phase=BAYANG_BIND;
	bind_counter = BAYANG_BIND_COUNT;
	BAYANG_initialize_txid();
	BAYANG_init();
	packet_count=0;
	return BAYANG_INITIAL_WAIT+BAYANG_PACKET_PERIOD;
}


// Convert 32b id to rx_tx_addr
void set_rx_tx_addr(uint32_t id)
{ // Used by almost all protocols
	rx_tx_addr[0] = (id >> 24) & 0xFF;
	rx_tx_addr[1] = (id >> 16) & 0xFF;
	rx_tx_addr[2] = (id >>  8) & 0xFF;
	rx_tx_addr[3] = (id >>  0) & 0xFF;
	rx_tx_addr[4] = (rx_tx_addr[2]&0xF0)|(rx_tx_addr[3]&0x0F);
}
