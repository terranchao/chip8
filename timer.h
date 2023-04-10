#ifndef TIMER_H
#define TIMER_H

#include <pthread.h>
#include <stdint.h>

extern volatile uint8_t g_timer_start;
extern uint8_t g_delay_timer;
extern uint8_t g_sound_timer;
extern pthread_mutex_t g_timer_mutex;
extern void *timer_fn(void *p);

#endif // TIMER_H
