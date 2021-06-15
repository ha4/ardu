#ifndef _TXUTIL_DATA_H_
#define _TXUTIL_DATA_H_

#define STATE_DATA 0
#define STATE_BIND 1
#define STATE_ERR  -1
#define STATE_DOINIT -2
#define STATE_DOBIND -3

struct proto_t {
  void (*init)();
  void (*bind)();
  uint16_t (*callback)();
  int8_t (*state)();
  void (*data)();
  char *name;
};

extern struct proto_t tx_lst[];
extern uint32_t MProtocol_id_master;
extern int8_t txstate;
extern struct proto_t *txproto;

void random_init(void);
uint32_t random_value(void);
uint32_t random_id(uint16_t address, uint8_t create_new); // 11 bytes

void set_protocol(char *name); // search and set 'txproto'
char *get_protocol(); // current proto name
char *next_protocol(); // cycle over protocols
char* saved_proto(uint16_t address, char *new_name); // 8 bytes

#endif
