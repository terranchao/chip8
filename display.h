#ifndef DISPLAY_H
#define DISPLAY_H

#include <pthread.h>
#include <stdint.h>

extern void clear_display();
extern uint8_t draw_sprite(
    size_t row,
    size_t col,
    const uint8_t *sprite_address,
    const size_t sprite_size
);
extern pthread_mutex_t g_display_mutex;
extern pthread_cond_t g_display_cond;
extern uint32_t g_background_color;
extern uint32_t g_foreground_color;

#endif // DISPLAY_H
