
class xtimer {
public:
  xtimer() { stop(); }
  ~xtimer() { }

  byte check(uint32_t t) {
    if (wait==0xFFFFFFFF) { t0 = t; return 0; }
    if (wait==0) { t0 = t; return 1; }
    if (t - t0 < wait)  return 0;
    t0 = t;
    return 1;
  }

  void set(uint32_t dt) { wait=dt; }
  void start(uint32_t now) { t0=now; }
  void stop() { set(0xFFFFFFFF); }

  uint32_t t0;
  uint32_t wait;
};


