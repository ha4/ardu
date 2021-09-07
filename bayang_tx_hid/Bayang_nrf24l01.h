#ifndef _BAYANG_INTERFACE_H_
#define _BAYANG_INTERFACE_H_

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
  uint8_t option;    // not transmitted
  uint8_t lqi;      // received telemetry LQI
};

void BAYANG_TX_init();
uint16_t BAYANG_TX_callback();
void BAYANG_TX_bind();

void BAYANG_TX_id(uint32_t id);
void BAYANG_TX_data(struct BayangData *x);
void BAYANG_TX_telemetry(uint16_t *tele);
int8_t BAYANG_TX_state(); // 0=binding, 1=data, -1=error

void noneTX_init();
uint16_t noneTX_callback();
void noneTX_bind();
int8_t  noneTX_state();


#endif //_INTERFACE_H_
