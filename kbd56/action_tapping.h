/* period of tapping(ms) */
#ifndef TAPPING_TERM
#define TAPPING_TERM    200
#endif

/* tap count needed for toggling a feature */
#ifndef TAPPING_TOGGLE
#define TAPPING_TOGGLE  5
#endif

#define WAITING_BUFFER_SIZE 8


#ifndef NO_ACTION_TAPPING
void action_tapping_process(keyrecord_t record);
#endif

