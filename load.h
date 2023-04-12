#ifndef LOAD_H
#define LOAD_H

#include <stdint.h>

extern char *g_romfile;
extern const uint16_t PROGRAM_START;
extern const size_t FONT_START;
extern const size_t FONT_SIZE;

extern void load_memory(uint8_t *memory);

#ifdef DEBUG
extern void print_memory(const uint8_t *memory);
#endif

#endif // LOAD_H
