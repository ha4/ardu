/* yd717_protocol.h -- Handle the yd717 protocol.
 *
 */

#ifndef YD717_PROTOCOL_H_
#define YD717_PROTOCOL_H_

#include "Arduino.h"
#include "nrf24l01p.h"


typedef struct __attribute__((__packed__)) {
  uint8_t throttle;
  int8_t yaw;
  int8_t pitch;
  int8_t roll;
  int8_t trim_yaw;
  int8_t trim_pitch;
  int8_t trim_roll;
  uint8_t flags;
} rx_values_t;


enum rxState
{
  NO_BIND = 0,
  WAIT_FIRST_SYNCHRO,
  BOUND,
  SIGNAL_LOST
};


enum rxReturn
{
  BOUND_NEW_VALUES = 0,   // Bound state, frame received with new TX values
  BOUND_NO_VALUES,        // Bound state, no new frame received
  NOT_BOUND,              // Not bound, initial state
  BIND_IN_PROGRESS,       // Bind in progress, first frame has been received with TX id, wait no bind frame
  ERROR_SIGNAL_LOST,      // Signal lost
  UNKNOWN                 // ???, not used for moment
};

class yd717Protocol
{
public:
  yd717Protocol();
  ~yd717Protocol();

  void printframe(char *str, byte sz);
  void print(char *str);
  void print(byte m);
  void print(int i);


  void init(nrf24l01p *wireless);
  uint8_t run(rx_values_t *rx_value );
  
protected:
  nrf24l01p *mWireless;
  uint32_t mID;
  uint8_t mTxid[3];
  uint8_t mRfChannels[16];
  uint8_t mRfChNum;
  uint8_t mFrame[16];
  uint8_t mState;
  unsigned long mLastSignalTime;
};

#endif /* YD717_PROTOCOL_H_ */
