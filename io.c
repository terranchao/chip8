/*
 * The functions in this file run in the main "I/O" thread. This thread is
 * responsible for initializing and cleaning up all of the aspects of the user
 * interface, including the application window and the display inside of it.
 * The remainder (and majority) of its time is spent polling for key input from
 * the user. Valid key input events signal the chip8 "CPU" thread, which then
 * processes those events.
 */
#include <SDL2/SDL.h>
#include <pthread.h>
#include <stdint.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "chip8.h"
#include "display.h"
#include "io.h"

volatile uint8_t g_io_done = 0;

/* Display */
size_t g_pixel_scale = 0;
static SDL_Window *g_window = NULL;
SDL_Renderer *g_renderer = NULL;
SDL_Texture *g_texture = NULL;
uint32_t *g_framebuffer = NULL;
size_t g_buffer_size = 0;
size_t g_width_in_bytes = 0;
const size_t DISPLAY_WIDTH = 64;
const size_t DISPLAY_HEIGHT = 32;

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
                pthread_mutex_lock(&g_input_mutex);
                g_io_done = 1;
                pthread_cond_signal(&g_input_cond);
                pthread_mutex_unlock(&g_input_mutex);
                return;
            }
        }
    }
}

void io_quit()
{
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
#ifdef DEBUG
    printf("%s\n", __func__);
#endif
}
