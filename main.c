
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "chip8.h"
#include "display.h"
#include "io.h"
#include "load.h"
#include "timer.h"

int main(int argc, char *argv[])
{
    const size_t pixel_scale_min = 1;
    const size_t pixel_scale_max = 120;
    const unsigned int delay_max = 10;

    if (argc != 4)
    {
        printf(
            "                                                               \n"
            "[USAGE] %s scale delay ROM                                     \n"
            "                                                               \n"
            "Arguments:                                                     \n"
            "    scale - window size multiplier (range: [%lu, %lu])         \n"
            "    delay - instruction cycle delay amount (range: [0, %u])    \n"
            "    ROM   - CHIP-8 program filename                            \n"
            "                                                               \n"
            "The recommended strategy for finding the right `scale` and `delay`"
            " values is to start them small, and slowly increase them until "
            "\"it feels right\".                                            \n"
            "A `scale` in the range of [10, 30] should do well for most "
            "users.                                                         \n"
            "The `delay` argument is to be used for tweaking the animation "
            "speed of a CHIP-8 program. Without an added delay, most modern "
            "processors will cause programs to run too quickly.             \n"
            "                                                               \n"
            , argv[0], pixel_scale_min, pixel_scale_max, delay_max
        );
        return 1;
    }

    g_pixel_scale = atoi(argv[1]);
    if ((g_pixel_scale < pixel_scale_min) || (g_pixel_scale > pixel_scale_max))
    {
        printf(
            "[ERROR] Argument 'scale' must be in range [%lu, %lu]\n",
            pixel_scale_min, pixel_scale_max
        );
        exit(EXIT_FAILURE);
    }

    g_delay = atoi(argv[2]);
    if (g_delay > delay_max)
    {
        printf(
            "[ERROR] Argument 'delay' must be in range [0, %u]\n", delay_max
        );
        exit(EXIT_FAILURE);
    }

    g_romfile = argv[3];

    pthread_t t1, t2;
    io_init();
    pthread_mutex_init(&g_display_mutex, NULL);
    pthread_mutex_init(&g_input_mutex, NULL);
    pthread_mutex_init(&g_timer_mutex, NULL);
    pthread_cond_init(&g_input_cond, NULL);
    pthread_create(&t1, NULL, timer_fn, NULL);
    pthread_create(&t2, NULL, chip8_fn, NULL);
    io_loop();
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_cond_destroy(&g_input_cond);
    pthread_mutex_destroy(&g_display_mutex);
    pthread_mutex_destroy(&g_input_mutex);
    pthread_mutex_destroy(&g_timer_mutex);
    io_quit();
    return 0;
}
