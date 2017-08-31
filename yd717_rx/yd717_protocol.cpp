#include "yd717_protocol.h"

/*****************************************************************/
/*	
    This portion of code is extracted from https://bitbucket.org/rivig/v202/src
    Thanks to rivig to his discovery and to share the v202 protocol.
*/

// This array stores 8 frequencies to test for the bind process
static uint8_t freq_test[8];

void yd717Protocol::printframe(char *str, byte sz)
{
  Serial.print(str);
  for(byte i=0; i < sz; i++) {
    if(mFrame[i] < 16) Serial.print('0');
    Serial.print(mFrame[i],HEX);
  }
  Serial.println('.');
}

void yd717Protocol::print(char *str)
{
  Serial.println(str);
}

void yd717Protocol::print(byte m)
{
  if(m < 16) Serial.print('0');
  Serial.println(m,HEX);
}

void yd717Protocol::print(int i)
{
  Serial.println(i);
}

yd717Protocol::yd717Protocol()
{
  mID=0x551C00AA; // random
  
  mState = NO_BIND;
  mLastSignalTime = 0;
  mRfChNum = 0;
}

yd717Protocol::~yd717Protocol()
{
}

void yd717Protocol::init(nrf24l01p *wireless)
{
  mWireless = wireless;
  mWireless->init(8); // PAYLOAD 8
  delayMicroseconds(100);
  mWireless->rxMode(0x3C);
  print(mWireless->readRegister(RF_CH));
}

// loop function, can be factorized (for later)
uint8_t yd717Protocol::run(rx_values_t *rx_value)
{
  uint8_t returnValue = UNKNOWN;
  static uint8_t errorCnt = 0;
  switch(mState) {
  case BOUND:
      returnValue = BOUND_NO_VALUES;
      if(!mWireless->rxFlag()) break;
      mWireless->resetRxFlag();

      while(!mWireless->rxEmpty()) {

        mWireless->readPayload(mFrame, 8);
        errorCnt = 0;
        // Discard bind frame
          if( mFrame[14] != 0xc0 ) {
            // Extract values
            returnValue = BOUND_NEW_VALUES;
            rx_value->throttle = mFrame[0];
            rx_value->yaw = mFrame[1] < 0x80 ? -mFrame[1] :  mFrame[1] - 0x80;
            rx_value->pitch = mFrame[2] < 0x80 ? -mFrame[2] :  mFrame[2] - 0x80;
            rx_value->roll = mFrame[3] < 0x80 ? -mFrame[3] :  mFrame[3] - 0x80;
            rx_value->trim_yaw = mFrame[4] - 0x40;
            rx_value->trim_pitch = mFrame[5] - 0x40;
            rx_value->trim_roll = mFrame[6] - 0x40;
            rx_value->flags = mFrame[14];
          }
      } // while
  break; // BOUND
  case NO_BIND:    // Initial state
      returnValue = NOT_BOUND;
 
      if(mWireless->rxFlag()) {
       print("rx!!");
        mWireless->resetRxFlag();
        while (!mWireless->rxEmpty())  {
          mWireless->readPayload(mFrame, 8);
          printframe("notbind ",8);
          mState = WAIT_FIRST_SYNCHRO;
          mWireless->flushRx();
          returnValue = BIND_IN_PROGRESS;
          break;
        }
      }  break; // NOBIND
  case WAIT_FIRST_SYNCHRO: // Wait on the first frequency of TX
      returnValue = BIND_IN_PROGRESS;
      if(!mWireless->rxFlag()) break;
      mWireless->resetRxFlag();
      while (!mWireless->rxEmpty())  {
        mWireless->readPayload(mFrame, 8);
        printframe("sync ", 8);
      }
  break;
  case SIGNAL_LOST: // Not implement for the moment
      returnValue = BIND_IN_PROGRESS;
  break;
  }
  
  return returnValue;
}


