
#include <pthread.h>
#include <stdio.h>

#include "chip8.h"
#include "display.h"
#include "io.h"
#include "load.h"
#include "timer.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("[USAGE] %s ROM\n", argv[0]);
        return 1;
    }

    g_romfile = argv[1];

    pthread_t t1, t2;
    io_init();
    pthread_mutex_init(&g_display_mutex, NULL);
    pthread_mutex_init(&g_input_mutex, NULL);
    pthread_mutex_init(&g_timer_mutex, NULL);
    pthread_cond_init(&g_input_cond, NULL);
    pthread_create(&t1, NULL, timer_fn, NULL);
    pthread_create(&t2, NULL, cpu_fn, NULL);
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
