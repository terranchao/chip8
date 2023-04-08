
#include <SDL2/SDL.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef DEBUG
#include <stdio.h>
#endif

#include "io.h"

volatile uint8_t g_done = 0;

/* Display output */
size_t g_pixel_scale = 0;
static SDL_Window *g_window = NULL;
static SDL_Renderer *g_renderer = NULL;
static SDL_Texture *g_texture = NULL;
static uint32_t *g_framebuffer = NULL;
static size_t g_buffer_size = 0;
static size_t g_width_in_bytes = 0;
static const size_t DISPLAY_WIDTH = 64;
static const size_t DISPLAY_HEIGHT = 32;
static const size_t SPRITE_WIDTH = 8;
static const uint32_t COLOR_UNSET = 0x00000000;
static const uint32_t COLOR_SET = 0xffffffff;
pthread_mutex_t g_display_mutex = {0};

/* Key input */
#define KEYMAP_SIZE 123
uint16_t g_keystate = 0;
uint8_t g_key_released = 0xff;
static uint8_t g_keymap[KEYMAP_SIZE] = {0};
const uint16_t g_bit16[16] =
{
    0x0001, 0x0002, 0x0004, 0x0008,
    0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800,
    0x1000, 0x2000, 0x4000, 0x8000,
};
pthread_mutex_t g_input_mutex = {0};
pthread_cond_t g_input_cond = {0};

/* Timers */
volatile uint8_t g_timer_start = 0;
uint8_t g_delay_timer = 0;
uint8_t g_sound_timer = 0;
pthread_mutex_t g_timer_mutex = {0};

#ifdef CLOCK_MONOTONIC_RAW
static const clockid_t clock_id = CLOCK_MONOTONIC_RAW;
#else
static const clockid_t clock_id = CLOCK_MONOTONIC;
#endif

void clear_display()
{
    pthread_mutex_lock(&g_display_mutex);
    memset(g_framebuffer, 0, g_buffer_size);
    pthread_mutex_unlock(&g_display_mutex);
}

static void update_display()
{
    pthread_mutex_lock(&g_display_mutex);
    SDL_UpdateTexture(
        g_texture,
        NULL,
        g_framebuffer,
        g_width_in_bytes
    );
    pthread_mutex_unlock(&g_display_mutex);
    SDL_RenderClear(g_renderer);
    SDL_RenderCopy(g_renderer, g_texture, NULL, NULL);
    SDL_RenderPresent(g_renderer);
}

static void draw_pixel(
    const size_t row, const size_t col, const uint8_t is_set, uint8_t *collision
)
{
    if (!is_set) return;

    // XOR
    size_t offset = (row*DISPLAY_WIDTH + col);
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
    // Implements full wrap
    row %= DISPLAY_HEIGHT;
    col %= DISPLAY_WIDTH;

    uint8_t collision = 0;
    pthread_mutex_lock(&g_display_mutex);
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

#ifdef DEBUG
static void benchmark_render(
    const size_t iterations,
    struct timespec *before,
    struct timespec *after,
    const char *target_name
)
{
    for (size_t i = 0; i < iterations; i++)
    {
        clock_gettime(clock_id, before);
        update_display();
        clock_gettime(clock_id, after);
        printf(
            "%s render time: %ld ns\n",
            target_name,
            ((after->tv_sec - before->tv_sec) * 1000000000 +
            (after->tv_nsec - before->tv_nsec))
        );
    }
}

static void benchmark_draw_sprite(
    const size_t iterations,
    struct timespec *before,
    struct timespec *after,
    const uint8_t *sprite_address,
    const size_t sprite_height,
    const char *target_name
)
{
    for (size_t i = 0; i < iterations; i++)
    {
        clear_display();
        clock_gettime(clock_id, before);
        draw_sprite(0, 0, sprite_address, sprite_height);
        clock_gettime(clock_id, after);
        printf(
            "%s draw time: %ld ns\n",
            target_name,
            ((after->tv_sec - before->tv_sec) * 1000000000 +
            (after->tv_nsec - before->tv_nsec))
        );
    }
}
#endif

static void handle_sdl_fatal(const char *message)
{
    SDL_LogError(
        SDL_LOG_CATEGORY_APPLICATION,
        "%s (%s)", message, SDL_GetError()
    );
    io_quit();
    exit(EXIT_FAILURE);
}

void io_init()
{
    g_buffer_size = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint32_t);
    g_framebuffer = (uint32_t*)malloc(g_buffer_size);
    g_width_in_bytes = DISPLAY_WIDTH * sizeof(uint32_t);

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        handle_sdl_fatal("Unable to initialize video");
    }
    g_window = SDL_CreateWindow(
        "A CHIP-8 Interpreter",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        g_pixel_scale * DISPLAY_WIDTH,
        g_pixel_scale * DISPLAY_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (!g_window)
    {
        handle_sdl_fatal("Unable to create window");
    }
    g_renderer = SDL_CreateRenderer(
        g_window,
        -1,
        SDL_RENDERER_ACCELERATED
    );
    if (!g_renderer)
    {
        handle_sdl_fatal("Unable to create renderer");
    }
    g_texture = SDL_CreateTexture(
        g_renderer,
        SDL_PIXELFORMAT_ARGB8888 , // fast
        SDL_TEXTUREACCESS_STREAMING,
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT
    );
    if (!g_texture)
    {
        handle_sdl_fatal("Unable to create texture");
    }

    // Set CHIP-8 key mappings
    memset(g_keymap, 0xff, KEYMAP_SIZE);
    g_keymap[SDLK_1] = 0x1;
    g_keymap[SDLK_2] = 0x2;
    g_keymap[SDLK_3] = 0x3;
    g_keymap[SDLK_4] = 0xc;
    g_keymap[SDLK_q] = 0x4;
    g_keymap[SDLK_w] = 0x5;
    g_keymap[SDLK_e] = 0x6;
    g_keymap[SDLK_r] = 0xd;
    g_keymap[SDLK_a] = 0x7;
    g_keymap[SDLK_s] = 0x8;
    g_keymap[SDLK_d] = 0x9;
    g_keymap[SDLK_f] = 0xe;
    g_keymap[SDLK_z] = 0xa;
    g_keymap[SDLK_x] = 0x0;
    g_keymap[SDLK_c] = 0xb;
    g_keymap[SDLK_v] = 0xf;
    
#ifdef DEBUG
    uint8_t debug_collision;
    struct timespec before, after;

    for (size_t i = 0; i < DISPLAY_HEIGHT; i++)
    {
        for (size_t j = 0; j < DISPLAY_WIDTH; j++)
        {
            draw_pixel(i, j, ((i+j)%2), &debug_collision);
        }
    }
    benchmark_render(40, &before, &after, "Grid");

    uint8_t debug_sprite[] = {
        0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
        0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x02,
        0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x40, 0x20,
        0x10, 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08,
    };
    benchmark_draw_sprite(
        20, &before, &after, debug_sprite, sizeof(debug_sprite), "Sprite"
    );
    benchmark_render(20, &before, &after, "Sprite");

    uint8_t debug_invert[] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    };
    draw_sprite(0, 0, debug_invert, sizeof(debug_invert));
#endif
}

void io_loop()
{
    while (1)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (
                ((e.type == SDL_KEYDOWN) || (e.type == SDL_KEYUP)) &&
                (e.key.keysym.sym < KEYMAP_SIZE) &&
                (g_keymap[e.key.keysym.sym] < 16)
            )
            {
                /* Keypad */
                if (e.type == SDL_KEYDOWN)
                {
                    pthread_mutex_lock(&g_input_mutex);
                    g_keystate |= g_bit16[g_keymap[e.key.keysym.sym]];
                    pthread_mutex_unlock(&g_input_mutex);
                }
                else
                {
                    // e.type == SDL_KEYUP
                    pthread_mutex_lock(&g_input_mutex);
                    g_keystate &= ~g_bit16[g_keymap[e.key.keysym.sym]];
                    g_key_released = g_keymap[e.key.keysym.sym];
                    pthread_cond_signal(&g_input_cond);
                    pthread_mutex_unlock(&g_input_mutex);
                }
#ifdef DEBUG
                printf("%X %d | g_keystate: %04x\n",
                    g_keymap[e.key.keysym.sym],
                    (e.type == SDL_KEYDOWN),
                    g_keystate
                );
#endif
            }
            else if (
                ((e.type == SDL_KEYUP) && (e.key.keysym.sym == SDLK_ESCAPE)) ||
                (e.type == SDL_QUIT)
            )
            {
                /* Quit */
                g_done = 1;
                return;
            }
        }
    }
}

void io_quit()
{
#ifdef DEBUG
    printf("Entered %s\n", __func__);
#endif
    if (g_framebuffer)
    {
        free(g_framebuffer);
        g_framebuffer = NULL;
    }
    if (g_texture)
    {
        SDL_DestroyTexture(g_texture);
        g_texture = NULL;
    }
    if (g_renderer)
    {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = NULL;
    }
    if (g_window)
    {
        SDL_DestroyWindow(g_window);
        g_window = NULL;
    }
    SDL_Quit();
}

void *timer_fn(__attribute__ ((unused)) void *p)
{
    g_timer_start = 1;

    const long period_ns = 16666667; // ~60Hz
    struct timespec before, after, sleep;
    while (!g_done)
    {
        clock_gettime(clock_id, &before);
        update_display();
        pthread_mutex_lock(&g_timer_mutex);
        if (g_delay_timer > 0)
            g_delay_timer--;
        if (g_sound_timer > 0)
            g_sound_timer--;
        pthread_mutex_unlock(&g_timer_mutex);
        clock_gettime(clock_id, &after);
        long remaining_ns =
            period_ns + (before.tv_sec - after.tv_sec) * 1000000000 +
            (before.tv_nsec - after.tv_nsec); // period - elapsed
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
