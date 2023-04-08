#ifndef IO_H
#define IO_H

#include <pthread.h>
#include <stdint.h>

extern volatile uint8_t g_done;

/* Display output */
extern size_t g_pixel_scale;
extern void clear_display();
extern uint8_t draw_sprite(
    size_t row,
    size_t col,
    const uint8_t *sprite_address,
    const size_t sprite_size
);
extern pthread_mutex_t g_display_mutex;

/* Key input */
extern uint16_t g_keystate;
extern uint8_t g_key_released;
extern const uint16_t g_bit16[16];
extern pthread_mutex_t g_key_mutex;
extern pthread_cond_t g_key_cond;

/* Timers */
extern volatile uint8_t g_timer_start;
extern uint8_t g_delay_timer;
extern uint8_t g_sound_timer;
extern pthread_mutex_t g_timer_mutex;
extern void *timer_fn(void *p);

extern void io_init();
extern void io_loop();
extern void io_quit();

#endif // IO_H
