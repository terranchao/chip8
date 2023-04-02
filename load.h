#ifndef LOAD_H
#define LOAD_H

#include <stdint.h>

#include "chip8.h"

extern char *g_romfile;
extern const uint16_t PROGRAM_START;

extern void load(chip8_t *c8);

#ifdef DEBUG
extern void print_memory(const chip8_t *c8);
#endif

#endif // LOAD_H
