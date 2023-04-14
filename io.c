/*
 * The functions in this file run in the main "I/O" thread. This thread is
 * responsible for handling the user interface, including the application
 * window, the display, and sound. The remainder (and majority) of its time is
 * spent polling for key input from the user. Valid key input events signal the
 * CPU thread, which then processes those events. All of these features are made
 * possible by the SDL development library.
 */
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "chip8.h"
#include "io.h"
#include "timer.h"

volatile uint8_t g_io_done = 0;
volatile uint8_t g_pause = 0;
volatile uint8_t g_restart = 0;

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
uint16_t g_keystate = 0;
uint8_t g_key_released = 0xff;
const uint16_t g_bit16[16] =
{
    0x0001, 0x0002, 0x0004, 0x0008,
    0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800,
    0x1000, 0x2000, 0x4000, 0x8000,
};
pthread_mutex_t g_input_mutex = {0};
pthread_cond_t g_input_cond = {0};

/* Sound */
SDL_AudioDeviceID g_audio_device_id = {0};
static uint64_t g_samples_played = 0;
static const float SOUND_VOLUME = 0.05;
static const float SOUND_FREQUENCY = 300.0;
static const float SOUND_SAMPLE_RATE = 44100.0;

static void handle_sdl_fatal(const char *message)
{
    SDL_LogError(
        SDL_LOG_CATEGORY_APPLICATION,
        "%s (%s)", message, SDL_GetError()
    );
    io_quit();
    exit(EXIT_FAILURE);
}

void audio_callback(
    __attribute__ ((unused)) void *user_data,
    uint8_t *stream,
    int num_bytes
)
{
    float *fstream = (float*)(stream);
    size_t num_samples = (num_bytes/8); // each sample is two 32-bit floats
    for(size_t i = 0; i < num_samples; ++i)
    {
        const double x =
            2.0 * M_PI * SOUND_FREQUENCY *
            (g_samples_played + i) / SOUND_SAMPLE_RATE;
        fstream[2*i + 0] = SOUND_VOLUME * sin(x); // L
        fstream[2*i + 1] = SOUND_VOLUME * sin(x); // R
    }
    g_samples_played += num_samples;
}

void io_init()
{
    g_buffer_size = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint32_t);
    g_framebuffer = (uint32_t*)malloc(g_buffer_size);
    g_width_in_bytes = DISPLAY_WIDTH * sizeof(uint32_t);

    if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO) < 0)
    {
        handle_sdl_fatal("Unable to initialize");
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
    
    SDL_AudioSpec audio_spec_want = {0}, audio_spec;
    audio_spec_want.freq     = (int)SOUND_SAMPLE_RATE;
    audio_spec_want.format   = AUDIO_F32;
    audio_spec_want.channels = 2;
    audio_spec_want.samples  = 512;
    audio_spec_want.callback = audio_callback;
    g_audio_device_id = SDL_OpenAudioDevice(
        NULL,
        0,
        &audio_spec_want,
        &audio_spec,
        SDL_AUDIO_ALLOW_FORMAT_CHANGE
    );
    if (!g_audio_device_id)
    {
        handle_sdl_fatal("Unable to open audio device");
    }
}

void io_loop()
{
    // Set CHIP-8 key mappings
    const uint8_t keymap[] =
    {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x01, 0x02, 0x03, 0x0c, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x07, 0xff, 0x0b, 0x09, 0x06, 0x0e, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x04, 0x0d, 0x08, 0xff, 0xff, 0x0f, 0x05,
        0x00, 0xff, 0x0a,
    };
    const int32_t KEYMAP_SIZE = sizeof(keymap);

    while (1)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (
                ((e.type == SDL_KEYDOWN) || (e.type == SDL_KEYUP)) &&
                (e.key.keysym.sym < KEYMAP_SIZE) &&
                (keymap[e.key.keysym.sym] < 16)
            )
            {
                /* Keypad */
                pthread_mutex_lock(&g_input_mutex);
                if (g_in_fx0a)
                {
                    pthread_mutex_lock(&g_timer_mutex);
                    g_sound_timer = 0x04;
                    pthread_mutex_unlock(&g_timer_mutex);
                }
                if (e.type == SDL_KEYDOWN)
                {
                    g_keystate |= g_bit16[keymap[e.key.keysym.sym]];
                }
                else
                {
                    // e.type == SDL_KEYUP
                    g_keystate &= ~g_bit16[keymap[e.key.keysym.sym]];
                    g_key_released = keymap[e.key.keysym.sym];
                    pthread_cond_signal(&g_input_cond);
                }
                pthread_mutex_unlock(&g_input_mutex);
#ifdef DEBUG
                printf("%X %d | g_keystate: %04x\n",
                    keymap[e.key.keysym.sym],
                    (e.type == SDL_KEYDOWN),
                    g_keystate
                );
#endif
            }
            else if ((e.type == SDL_KEYUP) && (e.key.keysym.sym == SDLK_SPACE))
            {
                /* Pause */
                pthread_mutex_lock(&g_input_mutex);
                g_pause ^= 1;
                pthread_cond_signal(&g_input_cond);
                pthread_mutex_unlock(&g_input_mutex);
            }
            else if (
                (e.type == SDL_KEYUP) && (e.key.keysym.sym == SDLK_BACKSPACE)
            )
            {
                /* Restart */
                pthread_mutex_lock(&g_input_mutex);
                g_restart = 1;
                pthread_cond_signal(&g_input_cond);
                pthread_mutex_unlock(&g_input_mutex);
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
    SDL_CloseAudioDevice(g_audio_device_id);
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
