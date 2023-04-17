#ifndef IO_H
#define IO_H

#include <SDL2/SDL.h>
#include <pthread.h>
#include <stdint.h>

#define DISPLAY_WIDTH   64
#define DISPLAY_HEIGHT  32

/* Display */
extern SDL_Renderer *g_renderer;
extern SDL_Texture *g_texture;
extern uint32_t *g_framebuffer;
extern size_t g_buffer_size;
extern size_t g_width_in_bytes;
extern const size_t DISPLAY_AREA;

/* Key input */
extern uint8_t *g_keystate;
extern uint8_t g_key_released;
extern const size_t g_keymap[];
extern pthread_mutex_t g_input_mutex;
extern pthread_cond_t g_input_cond;

/* Sound */
extern SDL_AudioDeviceID g_audio_device_id;

extern volatile uint8_t g_io_done;
extern volatile uint8_t g_pause;
extern volatile uint8_t g_restart;
extern void io_init();
extern void io_loop();
extern void io_quit();

#endif // IO_H
