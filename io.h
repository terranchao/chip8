#ifndef IO_H
#define IO_H

#include <SDL2/SDL.h>
#include <pthread.h>
#include <stdint.h>

/* Display */
extern size_t g_pixel_scale;
extern SDL_Renderer *g_renderer;
extern SDL_Texture *g_texture;
extern uint32_t *g_framebuffer;
extern size_t g_buffer_size;
extern size_t g_width_in_bytes;
extern const size_t DISPLAY_WIDTH;
extern const size_t DISPLAY_HEIGHT;

/* Key input */
extern uint16_t g_keystate;
extern uint8_t g_key_released;
extern const uint16_t g_bit16[16];
extern pthread_mutex_t g_input_mutex;
extern pthread_cond_t g_input_cond;

/* Sound */
extern SDL_AudioDeviceID g_audio_device_id;

extern volatile uint8_t g_io_done;
extern void io_init();
extern void io_loop();
extern void io_quit();

#endif // IO_H
