
#include <Stream.h>

class SerialSend : public Stream
{
private:
  volatile uint8_t *tx_port, *tx_ddr;
  uint8_t tx_mask, tx_nmask;

  static inline void tunedDelay(uint16_t delay);
public:
  // public methods
  SerialSend(uint8_t transmitPin);
  ~SerialSend() { }
  void begin(long speed);
  virtual size_t write(uint8_t byte);
  int peek() { return -1; }
  virtual int read() { return -1; }
  virtual int available() { return 0; }
  virtual void flush() { }
  using Print::write;
};
