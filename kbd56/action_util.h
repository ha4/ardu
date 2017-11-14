#ifdef __cplusplus
extern "C" {
#endif

extern report_keyboard_t *keyboard_report;

void send_keyboard_report(void);

/* key */
void add_key(uint8_t key);
void del_key(uint8_t key);
void clear_keys(void);

/* modifier */
uint8_t get_mods(void);
void add_mods(uint8_t mods);
void del_mods(uint8_t mods);
void set_mods(uint8_t mods);
void clear_mods(void);

/* weak modifier */
uint8_t get_weak_mods(void);
void add_weak_mods(uint8_t mods);
void del_weak_mods(uint8_t mods);
void set_weak_mods(uint8_t mods);
void clear_weak_mods(void);

/* oneshot modifier */
void set_oneshot_mods(uint8_t mods);
void clear_oneshot_mods(void);
void oneshot_toggle(void);
void oneshot_enable(void);
void oneshot_disable(void);

/* inspect */
uint8_t has_anykey(void);
uint8_t has_anymod(void);
uint8_t get_first_key(void);

#ifdef __cplusplus
}
#endif
