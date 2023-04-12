#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>

#define MEMORY_SIZE 0x1000  // 4KB (4096 bytes)

typedef struct
{
    /* Memory */
    uint8_t memory[MEMORY_SIZE];

    /* Registers */
    uint8_t V[16];  // data registers (V0-VF)
    uint16_t I;     // address register

    /* Control Flow */
    uint16_t program_counter;
    uint16_t stack[16];
    int8_t stack_pointer;

} chip8_t;

extern volatile uint8_t g_cpu_done;
extern volatile uint8_t g_in_fx0a;

extern unsigned int g_delay;

extern void *chip8_fn(void *p);

#endif // CHIP8_H
