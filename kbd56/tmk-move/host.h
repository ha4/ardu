
#ifdef __cplusplus
extern "C" {
#endif

#ifdef NKRO_ENABLE
extern bool keyboard_nkro;
#endif

extern uint8_t keyboard_idle;
extern uint8_t keyboard_protocol;


/* host driver */
void host_set_driver(host_driver_t *driver);
host_driver_t *host_get_driver(void);

/* host driver interface */
uint8_t host_keyboard_leds(void);
void host_keyboard_send(report_keyboard_t *report);
void host_mouse_send(report_mouse_t *report);
void host_system_send(uint16_t data);
void host_consumer_send(uint16_t data);

uint16_t host_last_system_report(void);
uint16_t host_last_consumer_report(void);

#ifdef __cplusplus
}
#endif

