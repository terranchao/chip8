#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>

#include "chip8.h"

extern void init_terminal();
extern void write_registers_to_terminal(
    const chip8_t *c8, const uint16_t instruction
);
extern void clear_terminal();
extern void quit_terminal();

#endif // TERMINAL_H
