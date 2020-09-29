#ifndef _INTERFACE_H_
#define _INTERFACE_H_

enum BAYANG_FLAGS {
	// flags going to packet[2]
	BAYANG_FLAG_RTH			= 0x01,
	BAYANG_FLAG_HEADLESS	= 0x02, 
	BAYANG_FLAG_FLIP		= 0x08,
	BAYANG_FLAG_VIDEO		= 0x10, 
	BAYANG_FLAG_PICTURE		= 0x20, 
	// flags going to packet[3]
	BAYANG_FLAG_INVERTED	= 0x80,			// inverted flight on Floureon H101
	BAYANG_FLAG_TAKE_OFF	= 0x20,			// take off / landing on X16 AH
	BAYANG_FLAG_EMG_STOP	= 0x04|0x08,	// 0x08 for VISUO XS809H-W-HD-G
};


enum BAYANG_OPTIONS {
  BAYANG_OPTION_BIND = 0x01,
  BAYANG_OPTION_TELEMETRY = 0x02,
  BAYANG_OPTION_TELEMETRY_READ = 0x04,
  BAYANG_OPTION_ANALOGAUX = 0x08,

};

uint16_t initBAYANG(void);
uint16_t BAYANG_callback();

void set_rx_tx_addr(uint32_t id);
void BAYANG_data(uint16_t *AETR1234);
void BAYANG_telemetry( uint16_t *tele);
void BAYANG_bind();

#endif //_INTERFACE_H_
