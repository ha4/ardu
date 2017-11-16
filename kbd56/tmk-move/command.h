
/* TODO: Refactoring */
typedef enum { ONESHOT, CONSOLE, MOUSEKEY } command_state_t;
extern command_state_t command_state;

/* This allows to extend commands. Return false when command is not processed. */
bool command_extra(uint8_t code);
bool command_console_extra(uint8_t code);

#ifdef COMMAND_ENABLE
bool command_proc(uint8_t code);
#else
#define command_proc(code)      false
#endif

