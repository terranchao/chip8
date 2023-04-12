/*
 * This file contains the code for the timer thread, which performs the
 * following tasks at a frequency of 60Hz as precisely as it can:
 * - Render the display to the screen.
 * - Decrement the internal system timers.
 * - Play tone if sound timer is nonzero.
 */
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <pthread.h>
#include <stdint.h>
#ifdef DEBUG
#include <stdio.h>
#endif

#include <time.h>
#ifdef CLOCK_MONOTONIC_RAW
static const clockid_t clock_id = CLOCK_MONOTONIC_RAW;
#else
static const clockid_t clock_id = CLOCK_MONOTONIC;
#endif

#include "chip8.h"
#include "display.h"
#include "io.h"
#include "timer.h"

volatile uint8_t g_timer_start = 0;
uint8_t g_delay_timer = 0;
uint8_t g_sound_timer = 0;
pthread_mutex_t g_timer_mutex = {0};

static void update_display()
{
    pthread_mutex_lock(&g_display_mutex);
    SDL_UpdateTexture(
        g_texture,
        NULL,
        g_framebuffer,
        g_width_in_bytes
    );
    pthread_cond_signal(&g_display_cond);
    pthread_mutex_unlock(&g_display_mutex);
    SDL_RenderClear(g_renderer);
    SDL_RenderCopy(g_renderer, g_texture, NULL, NULL);
    SDL_RenderPresent(g_renderer);
}

static void update_timers()
{
    pthread_mutex_lock(&g_timer_mutex);
    if (g_delay_timer > 0)
        g_delay_timer--;
    if (g_sound_timer > 0)
    {
        SDL_PauseAudioDevice(g_audio_device_id, 0);
        g_sound_timer--;
    }
    else
    {
        SDL_PauseAudioDevice(g_audio_device_id, 1);
    }
    pthread_mutex_unlock(&g_timer_mutex);
}

void *timer_fn(__attribute__ ((unused)) void *p)
{
    g_timer_start = 1;

    const long period_ns = 16666667; // ~60Hz
    struct timespec before, after, sleep;
    while (!g_cpu_done)
    {
        clock_gettime(clock_id, &before);
        update_display();
        update_timers();
        clock_gettime(clock_id, &after);
        long remaining_ns =
            period_ns + (before.tv_sec - after.tv_sec) * 1000000000 +
            (before.tv_nsec - after.tv_nsec); // (period - elapsed)
        sleep.tv_sec = (remaining_ns / 1000000000);
        sleep.tv_nsec = (remaining_ns % 1000000000);
#ifdef DEBUG
        printf("Remaining: %ld ns\n", remaining_ns);
#endif
        nanosleep(&sleep, NULL);
    }
#ifdef DEBUG
    printf("%s exit\n", __func__);
#endif
    pthread_exit(NULL);
}
