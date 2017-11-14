#include <Arduino.h>
#include <stdint.h>
//#include <avr/interrupt.h>
#include "keycode.h"

#include "report.h"
#include "host_driver.h"
#include "host.h"
#include "util.h"


#ifdef NKRO_ENABLE
bool keyboard_nkro = true;
#endif

static host_driver_t *driver;
static uint16_t last_system_report = 0;
static uint16_t last_consumer_report = 0;


void host_set_driver(host_driver_t *d)
{
    driver = d;
}

host_driver_t *host_get_driver(void)
{
    return driver;
}

uint8_t host_keyboard_leds(void)
{
    if (!driver) return 0;
    return (*driver->keyboard_leds)();
}
/* send report */
void host_keyboard_send(report_keyboard_t *report)
{
//    if (!driver) return;
 //   (*driver->send_keyboard)(report);

        Serial.print("keyboard: ");
        for (uint8_t i = 0; i < KEYBOARD_REPORT_SIZE; i++)
            Serial.print(report->raw[i],HEX);
        Serial.print("\n");
    
}

void host_mouse_send(report_mouse_t *report)
{
    if (!driver) return;
    (*driver->send_mouse)(report);
}

void host_system_send(uint16_t report)
{
    if (report == last_system_report) return;
    last_system_report = report;

    if (!driver) return;
    (*driver->send_system)(report);

        Serial.print("system: ");
        Serial.println(report,HEX);
}

void host_consumer_send(uint16_t report)
{
    if (report == last_consumer_report) return;
    last_consumer_report = report;

    if (!driver) return;
    (*driver->send_consumer)(report);

        Serial.print("consumer: ");
        Serial.println(report,HEX);
}

uint16_t host_last_system_report(void)
{
    return last_system_report;
}

uint16_t host_last_consumer_report(void)
{
    return last_consumer_report;
}
