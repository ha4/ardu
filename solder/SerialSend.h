
#include <Stream.h>

class SerialSend : public Stream
{
private:
  uint16_t _tx_delay;
  uint8_t pin;
  static inline void tunedDelay(uint16_t delay);
public:
  // public methods
  SerialSend(uint8_t transmitPin) {   pin=transmitPin; }
  ~SerialSend() { }
  void begin(long speed);
  virtual size_t write(uint8_t byte);
  int peek() { return -1; }
  virtual int read() { return -1; }
  virtual int available() { return 0; }
  virtual void flush() { }
  using Print::write;
};
