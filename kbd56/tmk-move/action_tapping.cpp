
#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

#include "config.h"

#include "keyboard.h"
#include "keycode.h"

#include "action_code.h"
#include "action_macro.h"
#include "action.h"

#include "action_layer.h"
#include "action_tapping.h"

#ifndef NO_ACTION_TAPPING

#define TIMER_DIFF(a, b, max)   ((a) >= (b) ?  (a) - (b) : (max) - (b) + (a))
#define TIMER_DIFF_8(a, b)      TIMER_DIFF(a, b, 0xFFU)
#define TIMER_DIFF_16(a, b)     TIMER_DIFF(a, b, 0xFFFFU)
#define TIMER_DIFF_32(a, b)     TIMER_DIFF(a, b, 0xFFFFFFFFUL)
#define TIMER_DIFF_RAW(a, b)    TIMER_DIFF_8(a, b)


#define IS_TAPPING()            !IS_NOEVENT(tapping_key.event)
#define IS_TAPPING_PRESSED()    (IS_TAPPING() && tapping_key.event.pressed)
#define IS_TAPPING_RELEASED()   (IS_TAPPING() && !tapping_key.event.pressed)
#define IS_TAPPING_KEY(k)       (IS_TAPPING() && KEYEQ(tapping_key.event.key, (k)))
#define WITHIN_TAPPING_TERM(e)  (TIMER_DIFF_16(e.time, tapping_key.event.time) < TAPPING_TERM)


static keyrecord_t tapping_key = {};
static keyrecord_t waiting_buffer[WAITING_BUFFER_SIZE] = {};
static uint8_t waiting_buffer_head = 0;
static uint8_t waiting_buffer_tail = 0;

static bool process_tapping(keyrecord_t *record);
static bool waiting_buffer_enq(keyrecord_t record);
static void waiting_buffer_clear(void);
static bool waiting_buffer_typed(keyevent_t event);
static void waiting_buffer_scan_tap(void);
static void debug_tapping_key(void);
static void debug_waiting_buffer(void);


void action_tapping_process(keyrecord_t record)
{
    if (process_tapping(&record)) {
    } else {
        if (!waiting_buffer_enq(record)) {
            // clear all in case of overflow.
            clear_keyboard();
            waiting_buffer_clear();
//            tapping_key = {0};
        }
    }

    // process waiting_buffer
    for (; waiting_buffer_tail != waiting_buffer_head; waiting_buffer_tail = (waiting_buffer_tail + 1) % WAITING_BUFFER_SIZE) {
        if (process_tapping(&waiting_buffer[waiting_buffer_tail])) {
        } else {
            break;
        }
    }
}


/* Tapping
 *
 * Rule: Tap key is typed(pressed and released) within TAPPING_TERM.
 *       (without interfering by typing other key)
 */
/* return true when key event is processed or consumed. */
bool process_tapping(keyrecord_t *keyp)
{
    keyevent_t event = keyp->event;

    // if tapping
    if (IS_TAPPING_PRESSED()) {
        if (WITHIN_TAPPING_TERM(event)) {
            if (tapping_key.tap.count == 0) {
                if (IS_TAPPING_KEY(event.key) && !event.pressed) {
                    // first tap!
                    tapping_key.tap.count = 1;
                    process_action(&tapping_key);

                    // copy tapping state
                    keyp->tap = tapping_key.tap;
                    // enqueue
                    return false;
                }
#if TAPPING_TERM >= 500
                /* Process a key typed within TAPPING_TERM
                 * This can register the key before settlement of tapping,
                 * useful for long TAPPING_TERM but may prevent fast typing.
                 */
                else if (IS_RELEASED(event) && waiting_buffer_typed(event)) {
                    process_action(&tapping_key);
                    tapping_key = (keyrecord_t){};
                    // enqueue
                    return false;
                }
#endif
                /* Process release event of a key pressed before tapping starts
                 * Without this unexpected repeating will occur with having fast repeating setting
                 * https://github.com/tmk/tmk_keyboard/issues/60
                 */
                else if (IS_RELEASED(event) && !waiting_buffer_typed(event)) {
                    // Modifier should be retained till end of this tapping.
                    action_t action = layer_switch_get_action(event);
                    switch (action.kind.id) {
                        case ACT_LMODS:
                        case ACT_RMODS:
                            if (action.key.mods && !action.key.code) return false;
                            if (IS_MOD(action.key.code)) return false;
                            break;
                        case ACT_LMODS_TAP:
                        case ACT_RMODS_TAP:
                            if (action.key.mods && keyp->tap.count == 0) return false;
                            if (IS_MOD(action.key.code)) return false;
                            break;
                    }
                    // Release of key should be process immediately.
                    process_action(keyp);
                    return true;
                }
                else {
                    // set interrupted flag when other key preesed during tapping
                    if (event.pressed) {
                        tapping_key.tap.interrupted = true;
                    }
                    // enqueue 
                    return false;
                }
            }
            // tap_count > 0
            else {
                if (IS_TAPPING_KEY(event.key) && !event.pressed) {
                    keyp->tap = tapping_key.tap;
                    process_action(keyp);
                    tapping_key = *keyp;
                    return true;
                }
                else if (is_tap_key(event) && event.pressed) {
                    if (tapping_key.tap.count > 1) {
                        // unregister key
                        keyrecord_t a = { {
                                /*.event.key =*/ tapping_key.event.key,
                                /*.event.pressed =*/ false,
                                /*.event.time =*/ event.time },
                                 /* .tap =*/ tapping_key.tap
                        };
                        process_action(&a);
                    } else {
                    }
                    tapping_key = *keyp;
                    waiting_buffer_scan_tap();
                    return true;
                }
                else {
                    process_action(keyp);
                    return true;
                }
            }
        }
        // after TAPPING_TERM
        else {
            if (tapping_key.tap.count == 0) {
                process_action(&tapping_key);
//                tapping_key = (keyrecord_t){};
                return false;
            }  else {
                if (IS_TAPPING_KEY(event.key) && !event.pressed) {
                    keyp->tap = tapping_key.tap;
                    process_action(keyp);
//                    tapping_key = (keyrecord_t){};
                    return true;
                }
                else if (is_tap_key(event) && event.pressed) {
                    if (tapping_key.tap.count > 1) {
                        // unregister key
                        keyrecord_t a = { {
                                /*.event.key =*/ tapping_key.event.key,
                                /*.event.pressed =*/ false,
                                /*.event.time =*/ event.time },
                                /* .tap = */ tapping_key.tap,
                        };
                        process_action(&a);
                    } else {
                    }
                    tapping_key = *keyp;
                    waiting_buffer_scan_tap();
                    return true;
                }
                else {
                    process_action(keyp);
                    return true;
                }
            }
        }
    } else if (IS_TAPPING_RELEASED()) {
        if (WITHIN_TAPPING_TERM(event)) {
            if (event.pressed) {
                if (IS_TAPPING_KEY(event.key)) {
                    if (!tapping_key.tap.interrupted && tapping_key.tap.count > 0) {
                        // sequential tap.
                        keyp->tap = tapping_key.tap;
                        if (keyp->tap.count < 15) keyp->tap.count += 1;
                        process_action(keyp);
                        tapping_key = *keyp;
                        return true;
                    } else {
                        // FIX: start new tap again
                        tapping_key = *keyp;
                        return true;
                    }
                } else if (is_tap_key(event)) {
                    // Sequential tap can be interfered with other tap key.
                    tapping_key = *keyp;
                    waiting_buffer_scan_tap();
                    return true;
                } else {
                    // should none in buffer
                    // FIX: interrupted when other key is pressed
                    tapping_key.tap.interrupted = true;
                    process_action(keyp);
                    return true;
                }
            } else {
                process_action(keyp);
                return true;
            }
        } else {
            // FIX: process_aciton here?
            // timeout. no sequential tap.
            //tapping_key = (keyrecord_t){};
            return false;
        }
    }
    // not tapping state
    else {
        if (event.pressed && is_tap_key(event)) {
            tapping_key = *keyp;
            waiting_buffer_scan_tap();
            return true;
        } else {
            process_action(keyp);
            return true;
        }
    }
}


/*
 * Waiting buffer
 */
bool waiting_buffer_enq(keyrecord_t record)
{
    if (IS_NOEVENT(record.event)) {
        return true;
    }

    if ((waiting_buffer_head + 1) % WAITING_BUFFER_SIZE == waiting_buffer_tail) {
        return false;
    }

    waiting_buffer[waiting_buffer_head] = record;
    waiting_buffer_head = (waiting_buffer_head + 1) % WAITING_BUFFER_SIZE;

    return true;
}

void waiting_buffer_clear(void)
{
    waiting_buffer_head = 0;
    waiting_buffer_tail = 0;
}

bool waiting_buffer_typed(keyevent_t event)
{
    for (uint8_t i = waiting_buffer_tail; i != waiting_buffer_head; i = (i + 1) % WAITING_BUFFER_SIZE) {
        if (KEYEQ(event.key, waiting_buffer[i].event.key) && event.pressed !=  waiting_buffer[i].event.pressed) {
            return true;
        }
    }
    return false;
}

/* scan buffer for tapping */
void waiting_buffer_scan_tap(void)
{
    // tapping already is settled
    if (tapping_key.tap.count > 0) return;
    // invalid state: tapping_key released && tap.count == 0
    if (!tapping_key.event.pressed) return;

    for (uint8_t i = waiting_buffer_tail; i != waiting_buffer_head; i = (i + 1) % WAITING_BUFFER_SIZE) {
        if (IS_TAPPING_KEY(waiting_buffer[i].event.key) &&
                !waiting_buffer[i].event.pressed &&
                WITHIN_TAPPING_TERM(waiting_buffer[i].event)) {
            tapping_key.tap.count = 1;
            waiting_buffer[i].tap.count = 1;
            process_action(&tapping_key);

            return;
        }
    }
}


#endif
