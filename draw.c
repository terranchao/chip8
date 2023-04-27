/*
 * The functions in this file are called from the CPU thread. They write to the
 * display's framebuffer, and then the timer thread renders the framebuffer to
 * the user. `pthread_cond_wait()` is used to enforce a maximum call frequency
 * to these functions, which is determined by the timer thread.
 */

#include <pthread.h>
#include <stdint.h>
#include <string.h>

#include "color.h"
#include "draw.h"
#include "io.h"

pthread_mutex_t g_display_mutex = {0};
pthread_cond_t g_display_cond = {0};

static const size_t DISPLAY_WIDTH_MASK = (DISPLAY_WIDTH-1);
static const size_t DISPLAY_HEIGHT_MASK = (DISPLAY_HEIGHT-1);

void clear_display()
{
    pthread_mutex_lock(&g_display_mutex);
    pthread_cond_wait(&g_display_cond, &g_display_mutex);
    for (size_t i = 0; i < DISPLAY_AREA; i++)
    {
        g_framebuffer[i] = g_background_color;
    }
    pthread_mutex_unlock(&g_display_mutex);
}

static void draw_pixel(
    const size_t row, const size_t col, const uint8_t is_set, uint8_t *collision
)
{
    if (!is_set) return;

    const size_t offset = (row*DISPLAY_WIDTH + col);

    // XOR
    if (g_framebuffer[offset] == g_foreground_color)
    {
        g_framebuffer[offset] = g_background_color;
        *collision = 1;
    }
    else
    {
        g_framebuffer[offset] = g_foreground_color;
    }
}

uint8_t draw_sprite(
    size_t row,
    size_t col,
    const uint8_t *sprite_address,
    const size_t sprite_height
)
{
    // Implements full sprite wrap
    row &= DISPLAY_HEIGHT_MASK;
    col &= DISPLAY_WIDTH_MASK;

    uint8_t collision = 0;
    pthread_mutex_lock(&g_display_mutex);
    pthread_cond_wait(&g_display_cond, &g_display_mutex);
    for (size_t i = 0; i < sprite_height; i++)
    {
        if ((row+i) > DISPLAY_HEIGHT_MASK) break;
        uint8_t line = sprite_address[i];
        for (size_t j = 0; j < 8; j++)
        {
            if ((col+j) > DISPLAY_WIDTH_MASK) break;
            draw_pixel(row+i, col+j, (line & 0x80), &collision);
            line <<= 1;
        }
    }
    pthread_mutex_unlock(&g_display_mutex);
    return collision;
}

static const uint8_t pause_icon[] = {
    0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc
};
inline void draw_pause_icon()
{
    draw_sprite(12, 29, pause_icon, sizeof(pause_icon));
}

static const uint8_t restart_icon[] = {
    0x00, 0x08, 0x18, 0x3f, 0x7f, 0x3f, 0x18, 0x08
};
inline void draw_restart_icon()
{
    draw_sprite(0, 0, restart_icon, sizeof(restart_icon));
}
