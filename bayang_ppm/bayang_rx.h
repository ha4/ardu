#ifndef _INTERFACE_RX_H_
#define _INTERFACE_RX_H_

#define RF_POWER TX_POWER_158mW

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
  BAYANG_OPTION_BIND = 0x01,  // auto binding
  BAYANG_OPTION_TELEMETRY = 0x02,
  BAYANG_OPTION_TELEMETRY_READ = 0x04,
  BAYANG_OPTION_ANALOGAUX = 0x08,

};

struct BayangData {
  uint16_t roll;      // aileron
  uint16_t pitch;     // elevator
  uint16_t throttle;  // throttle
  uint16_t yaw;       // rudder
  int8_t  trims[4];   // +/-31 AETR
  uint8_t aux1;
  uint8_t aux2;
  uint8_t flags2;
  uint8_t flags3;
  uint8_t option; // not transmitted
  uint8_t lqi;
};

void BAYANG_RX_data(struct BayangData *t);
uint8_t BAYANG_RX_available();
void BAYANG_RX_init();
void BAYANG_RX_bind();
int BAYANG_RX_state(); // 0:binding 1:data -1:data loss
uint16_t BAYANG_RX_callback();

#endif //_INTERFACE_H_
