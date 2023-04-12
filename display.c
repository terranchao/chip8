/*
 * The functions in this file are called from the CPU thread. They write to the
 * display's framebuffer, and then the timer thread renders the framebuffer to
 * the user. `pthread_cond_wait()` is used to enforce a maximum call frequency
 * to these functions, which is determined by the timer thread.
 */

#include <pthread.h>
#include <stdint.h>
#include <string.h>

#include "display.h"
#include "io.h"

pthread_mutex_t g_display_mutex = {0};
pthread_cond_t g_display_cond = {0};

static const size_t DISPLAY_WIDTH_MASK = 0x3f;  // 63
static const size_t DISPLAY_HEIGHT_MASK = 0x1f; // 31
static const size_t SPRITE_WIDTH = 8;
static const uint32_t COLOR_UNSET = 0x00000000;
static const uint32_t COLOR_SET = 0xffffffff;

void clear_display()
{
    pthread_mutex_lock(&g_display_mutex);
    pthread_cond_wait(&g_display_cond, &g_display_mutex);
    memset(g_framebuffer, 0, g_buffer_size);
    pthread_mutex_unlock(&g_display_mutex);
}

static void draw_pixel(
    const size_t row, const size_t col, const uint8_t is_set, uint8_t *collision
)
{
    if (!is_set) return;

    size_t offset = (row*DISPLAY_WIDTH + col);

    // XOR
    if (g_framebuffer[offset])
    {
        g_framebuffer[offset] = COLOR_UNSET;
        *collision = 1;
    }
    else
    {
        g_framebuffer[offset] = COLOR_SET;
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
        if ((row+i) > DISPLAY_HEIGHT) break;
        uint8_t line = sprite_address[i];
        for (size_t j = 0; j < SPRITE_WIDTH; j++)
        {
            if ((col+j) > DISPLAY_WIDTH) break;
            draw_pixel(row+i, col+j, (line & 0x80), &collision);
            line <<= 1;
        }
    }
    pthread_mutex_unlock(&g_display_mutex);
    return collision;
}
