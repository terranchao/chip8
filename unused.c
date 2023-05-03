/*
 * This code is not compiled - it was once used to test draw functions and
 * timing.
 */
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "draw.h"
#include "timer.h"

void benchmark_render(
    const size_t iterations,
    const char *target_name
)
{
    struct timespec before, after;
    for (size_t i = 0; i < iterations; i++)
    {
        clock_gettime(clock_id, &before);
        update_display();
        clock_gettime(clock_id, &after);
        printf(
            "%s render time: %ld ns\n",
            target_name,
            ((after.tv_sec - before.tv_sec) * 1000000000 +
            (after.tv_nsec - before.tv_nsec))
        );
    }
}

void benchmark_draw_sprite(
    const size_t iterations,
    const uint8_t *sprite_address,
    const size_t sprite_height,
    const char *target_name
)
{
    struct timespec before, after;
    for (size_t i = 0; i < iterations; i++)
    {
        clear_display();
        clock_gettime(clock_id, &before);
        draw_sprite(0, 0, sprite_address, sprite_height);
        clock_gettime(clock_id, &after);
        printf(
            "%s draw time: %ld ns\n",
            target_name,
            ((after.tv_sec - before.tv_sec) * 1000000000 +
            (after.tv_nsec - before.tv_nsec))
        );
    }
}

void test_draw()
{
    uint8_t collision;
    for (size_t i = 0; i < DISPLAY_HEIGHT; i++)
    {
        for (size_t j = 0; j < DISPLAY_WIDTH; j++)
        {
            draw_pixel(i, j, ((i+j)%2), &collision);
        }
    }
    benchmark_render(40, "Grid");

    const uint8_t sprite[] = {
        0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
        0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x02,
        0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x40, 0x20,
        0x10, 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08,
    };
    benchmark_draw_sprite(
        20, sprite, sizeof(sprite), "Sprite"
    );
    benchmark_render(20, "Sprite");

    const uint8_t invert[] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    };
    draw_sprite(0, 0, invert, sizeof(invert));
}
