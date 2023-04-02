
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "chip8.h"
#include "io.h"
#include "load.h"

unsigned int g_delay = 0;

static const char *DEST_ADDR_OOR = "Destination address is out of range";
static const char *DEST_ADDR_SELF =
    "Self-destination address will result in an infinite loop";

static inline void handle_error(
    const char *message, const uint16_t bad_address, const uint16_t instruction
)
{
    printf(
        "[ERROR] %s (Memory[0x%03x]: 0x%04x)\n",
        message, bad_address, instruction
    );
    exit(EXIT_FAILURE);
}

static void undefined_instruction(chip8_t *c8, const uint16_t instruction)
{
    handle_error(
        "Encountered undefined instruction",
        c8->program_counter-2, instruction
    );
}

static void execute_0nnn(chip8_t *c8, const uint16_t instruction)
{
#ifdef LEGACY
    const uint16_t address = (instruction & 0x0fff);
#endif
    switch (instruction)
    {
        case 0x00e0:
            clear_display();
            break;
        case 0x00ee:
            // Return from subroutine
            if (c8->stack_pointer == -1)
            {
                handle_error(
                    "Trying to decrement stack pointer beyond limit",
                    c8->program_counter-2, instruction
                );
            }
            c8->program_counter = c8->stack[c8->stack_pointer];
            c8->stack_pointer--;
            break;
        default:
#ifdef LEGACY
            // Jump to machine code routine
            if (address == (c8->program_counter-2))
            {
                handle_error(
                    DEST_ADDR_SELF, c8->program_counter-2, instruction
                );
            }
            c8->program_counter = address;
#else
            undefined_instruction(c8, instruction);
#endif
            break;
    }
}

static void execute_1nnn(chip8_t *c8, const uint16_t instruction)
{
    // Jump to address
    const uint16_t address = (instruction & 0x0fff);
    if (address < PROGRAM_START)
    {
        handle_error(DEST_ADDR_OOR, c8->program_counter-2, instruction);
    }
    if (address == (c8->program_counter-2))
    {
        handle_error(DEST_ADDR_SELF, c8->program_counter-2, instruction);
    }
    c8->program_counter = address;
}

static void execute_2nnn(chip8_t *c8, const uint16_t instruction)
{
    // Call subroutine
    if (c8->stack_pointer == 15)
    {
        handle_error(
            "Trying to increment stack pointer beyond limit",
            c8->program_counter-2, instruction
        );
    }
    const uint16_t address = (instruction & 0x0fff);
    if (address < PROGRAM_START)
    {
        handle_error(DEST_ADDR_OOR, c8->program_counter-2, instruction);
    }
    if (address == (c8->program_counter-2))
    {
        handle_error(DEST_ADDR_SELF, c8->program_counter-2, instruction);
    }
    c8->stack_pointer++;
    c8->stack[c8->stack_pointer] = c8->program_counter;
    c8->program_counter = address;
}

static void execute_3xnn(chip8_t *c8, const uint16_t instruction)
{
    // Skip next instruction if Vx == byte
    if (c8->V[(instruction & 0x0f00) >> 8] == (instruction & 0x00ff))
    {
        c8->program_counter += 2;
    }
}

static void execute_4xnn(chip8_t *c8, const uint16_t instruction)
{
    // Skip next instruction if Vx != byte
    if (c8->V[(instruction & 0x0f00) >> 8] != (instruction & 0x00ff))
    {
        c8->program_counter += 2;
    }
}

static void execute_5xy0(chip8_t *c8, const uint16_t instruction)
{
    // Skip next instruction if Vx == Vy
    if (
        c8->V[(instruction & 0x0f00) >> 8] == c8->V[(instruction & 0x00f0) >> 4]
    )
    {
        c8->program_counter += 2;
    }
}

static void execute_6xnn(chip8_t *c8, const uint16_t instruction)
{
    // Vx = byte
    c8->V[(instruction & 0x0f00) >> 8] = (instruction & 0x00ff);
}

static void execute_7xnn(chip8_t *c8, const uint16_t instruction)
{
    // Vx += byte
    c8->V[(instruction & 0x0f00) >> 8] += (instruction & 0x00ff);
}

static void execute_8xy0(chip8_t *c8, const uint16_t instruction)
{
    // Vx = Vy
    c8->V[(instruction & 0x0f00) >> 8] = c8->V[(instruction & 0x00f0) >> 4];
}

static void execute_8xy1(chip8_t *c8, const uint16_t instruction)
{
    // Vx |= Vy
    c8->V[(instruction & 0x0f00) >> 8] |= c8->V[(instruction & 0x00f0) >> 4];
}

static void execute_8xy2(chip8_t *c8, const uint16_t instruction)
{
    // Vx &= Vy
    c8->V[(instruction & 0x0f00) >> 8] &= c8->V[(instruction & 0x00f0) >> 4];
}

static void execute_8xy3(chip8_t *c8, const uint16_t instruction)
{
    // Vx ^= Vy
    c8->V[(instruction & 0x0f00) >> 8] ^= c8->V[(instruction & 0x00f0) >> 4];
}

static void execute_8xy4(chip8_t *c8, const uint16_t instruction)
{
    // Vx += Vy
    const uint8_t x = ((instruction & 0x0f00) >> 8);
    const uint8_t before = c8->V[x];
    c8->V[x] += c8->V[(instruction & 0x00f0) >> 4];
    c8->V[0xf] = (c8->V[x] < before) ? 1 : 0;
}

static void execute_8xy5(chip8_t *c8, const uint16_t instruction)
{
    // Vx -= Vy
    const uint8_t x = ((instruction & 0x0f00) >> 8);
    const uint8_t y = ((instruction & 0x00f0) >> 4);
    c8->V[0xf] = (c8->V[x] > c8->V[y]) ? 1 : 0;
    c8->V[x] -= c8->V[y];
}

static void execute_8xy6(chip8_t *c8, const uint16_t instruction)
{
#ifdef LEGACY
    // Vx = (Vy >>= 1)
    const uint8_t y = ((instruction & 0x00f0) >> 4);
    c8->V[0xf] = (c8->V[y] & 0x01);
    c8->V[y] >>= 1;
    c8->V[(instruction & 0x0f00) >> 8] = c8->V[y];
#else
    // Vx >>= 1
    const uint8_t x = ((instruction & 0x0f00) >> 8);
    c8->V[0xf] = (c8->V[x] & 0x01);
    c8->V[x] >>= 1;
#endif
}

static void execute_8xy7(chip8_t *c8, const uint16_t instruction)
{
    // Vx = (Vy - Vx)
    const uint8_t x = ((instruction & 0x0f00) >> 8);
    const uint8_t y = ((instruction & 0x00f0) >> 4);
    c8->V[0xf] = (c8->V[y] > c8->V[x]) ? 1 : 0;
    c8->V[x] = (c8->V[y] - c8->V[x]);
}

static void execute_8xye(chip8_t *c8, const uint16_t instruction)
{
#ifdef LEGACY
    // Vx = (Vy <<= 1)
    const uint8_t y = ((instruction & 0x00f0) >> 4);
    c8->V[0xf] = ((c8->V[y] & 0x80) >> 7);
    c8->V[y] <<= 1;
    c8->V[(instruction & 0x0f00) >> 8] = c8->V[y];
#else
    // Vx <<= 1
    const uint8_t x = ((instruction & 0x0f00) >> 8);
    c8->V[0xf] = ((c8->V[x] & 0x80) >> 7);
    c8->V[x] <<= 1;
#endif
}

static void (* const g_execute_8nnn[16])(chip8_t*, const uint16_t) = 
{
    execute_8xy0,
    execute_8xy1,
    execute_8xy2,
    execute_8xy3,
    execute_8xy4,
    execute_8xy5,
    execute_8xy6,
    execute_8xy7,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    execute_8xye,
    undefined_instruction,
};

static void execute_8nnn(chip8_t *c8, const uint16_t instruction)
{
    (g_execute_8nnn[instruction & 0x000f])(c8, instruction);
}

static void execute_9xy0(chip8_t *c8, const uint16_t instruction)
{
    // Skip next instruction if Vx != Vy
    if (
        c8->V[(instruction & 0x0f00) >> 8] != c8->V[(instruction & 0x00f0) >> 4]
    )
    {
        c8->program_counter += 2;
    }
}

static void execute_annn(chip8_t *c8, const uint16_t instruction)
{
    // Set I
    c8->I = (instruction & 0x0fff);
}

static void execute_bnnn(chip8_t *c8, const uint16_t instruction)
{
#if SUPER
    // Jump to address + V
    const uint16_t address =
        c8->V[(instruction & 0x0f00) >> 8] + (instruction & 0x0fff);
#else
    // Jump to address + V0
    const uint16_t address = c8->V[0x0] + (instruction & 0x0fff);
#endif
    if ((address < PROGRAM_START) || (address >= MEMORY_SIZE))
    {
        handle_error(DEST_ADDR_OOR, c8->program_counter-2, instruction);
    }
    if (address == (c8->program_counter-2))
    {
        handle_error(DEST_ADDR_SELF, c8->program_counter-2, instruction);
    }
    c8->program_counter = address;
}

static void execute_cxnn(chip8_t *c8, const uint16_t instruction)
{
    // Vx = random
    c8->V[(instruction & 0x0f00) >> 8] =
        ((rand() % 0x100) & (instruction & 0x00ff));
}

static void execute_dxyn(chip8_t *c8, const uint16_t instruction)
{
    // Draw sprite
    c8->V[0xf] = draw_sprite(
        c8->V[(instruction & 0x00f0) >> 4],
        c8->V[(instruction & 0x0f00) >> 8],
        &c8->memory[c8->I],
        instruction & 0x000f
    );
}

static void execute_ex9e(chip8_t *c8, const uint16_t instruction)
{
    // Skip next instruction if key in Vx is pressed
    if ((instruction & 0x00f0) != 0x0090)
    {
        undefined_instruction(c8, instruction);
    }
    pthread_mutex_lock(&g_key_mutex);
    if (g_keystate & g_bit16[c8->V[(instruction & 0x0f00) >> 8]])
    {
        c8->program_counter += 2;
    }
    pthread_mutex_unlock(&g_key_mutex);
}

static void execute_exa1(chip8_t *c8, const uint16_t instruction)
{
    // Skip next instruction if key in Vx is not pressed
    if ((instruction & 0x00f0) != 0x00a0)
    {
        undefined_instruction(c8, instruction);
    }
    pthread_mutex_lock(&g_key_mutex);
    if (!(g_keystate & g_bit16[c8->V[(instruction & 0x0f00) >> 8]]))
    {
        c8->program_counter += 2;
    }
    pthread_mutex_unlock(&g_key_mutex);
}

static void (* const g_execute_exnn[16])(chip8_t*, const uint16_t) = 
{
    undefined_instruction,
    execute_exa1,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    execute_ex9e,
    undefined_instruction,
};

static void execute_ennn(chip8_t *c8, const uint16_t instruction)
{
    (g_execute_exnn[instruction & 0x000f])(c8, instruction);
}

static void execute_fx07(chip8_t *c8, const uint16_t instruction)
{
    if ((instruction & 0x00f0) != 0x0000)
    {
        undefined_instruction(c8, instruction);
    }
    pthread_mutex_lock(&g_timer_mutex);
    c8->V[(instruction & 0x0f00) >> 8] = g_delay_timer;
    pthread_mutex_unlock(&g_timer_mutex);
}

static void execute_fx0a(chip8_t *c8, const uint16_t instruction)
{
    if ((instruction & 0x00f0) != 0x0000)
    {
        undefined_instruction(c8, instruction);
    }
    while (1)
    {
        pthread_mutex_lock(&g_key_mutex);
        if (g_keystate)
        {
            c8->V[(instruction & 0x0f00) >> 8] = g_last_pressed;
            pthread_mutex_unlock(&g_key_mutex);
            return;
        }
        pthread_mutex_unlock(&g_key_mutex);
    }
}

static void execute_fx15(chip8_t *c8, const uint16_t instruction)
{
    if ((instruction & 0x00f0) != 0x0010)
    {
        undefined_instruction(c8, instruction);
    }
    pthread_mutex_lock(&g_timer_mutex);
    g_delay_timer = c8->V[(instruction & 0x0f00) >> 8];
    pthread_mutex_unlock(&g_timer_mutex);
}

static void execute_fx18(chip8_t *c8, const uint16_t instruction)
{
    if ((instruction & 0x00f0) != 0x0010)
    {
        undefined_instruction(c8, instruction);
    }
    pthread_mutex_lock(&g_timer_mutex);
    g_sound_timer = c8->V[(instruction & 0x0f00) >> 8];
    pthread_mutex_unlock(&g_timer_mutex);
}

static void execute_fx1e(chip8_t *c8, const uint16_t instruction)
{
    if ((instruction & 0x00f0) != 0x0010)
    {
        undefined_instruction(c8, instruction);
    }
    (void)c8;
    (void)instruction;
}

static void execute_fx29(chip8_t *c8, const uint16_t instruction)
{
    if ((instruction & 0x00f0) != 0x0020)
    {
        undefined_instruction(c8, instruction);
    }
    (void)c8;
    (void)instruction;
}

static void execute_fx33(chip8_t *c8, const uint16_t instruction)
{
    if ((instruction & 0x00f0) != 0x0030)
    {
        undefined_instruction(c8, instruction);
    }
    (void)c8;
    (void)instruction;
}

static void execute_fx55(chip8_t *c8, const uint16_t instruction)
{
    if ((instruction & 0x00f0) != 0x0050)
    {
        undefined_instruction(c8, instruction);
    }
}

static void execute_fx65(chip8_t *c8, const uint16_t instruction)
{
    if ((instruction & 0x00f0) != 0x0060)
    {
        undefined_instruction(c8, instruction);
    }
}

static void (* const g_execute_fxn5[16])(chip8_t*, const uint16_t) = 
{
    undefined_instruction,
    execute_fx15,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    execute_fx55,
    execute_fx65,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
};

static void execute_fxn5(chip8_t *c8, const uint16_t instruction)
{
    (g_execute_fxn5[(instruction & 0x00f0) >> 4])(c8, instruction);
}

static void (* const g_execute_fxnn[16])(chip8_t*, const uint16_t) = 
{
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    execute_fx33,
    undefined_instruction,
    execute_fxn5,
    undefined_instruction,
    execute_fx07,
    execute_fx18,
    execute_fx29,
    execute_fx0a,
    undefined_instruction,
    undefined_instruction,
    undefined_instruction,
    execute_fx1e,
    undefined_instruction,
};

static void execute_fnnn(chip8_t *c8, const uint16_t instruction)
{
    (g_execute_fxnn[instruction & 0x000f])(c8, instruction);
}

static void (* const g_execute[16])(chip8_t*, const uint16_t) = 
{
    execute_0nnn,
    execute_1nnn,
    execute_2nnn,
    execute_3xnn,
    execute_4xnn,
    execute_5xy0,
    execute_6xnn,
    execute_7xnn,
    execute_8nnn,
    execute_9xy0,
    execute_annn,
    execute_bnnn,
    execute_cxnn,
    execute_dxyn,
    execute_ennn,
    execute_fnnn,
};

static void run(chip8_t *c8)
{
    while (!g_done)
    {
        // Fetch
        const uint16_t instruction =
            (c8->memory[c8->program_counter] << 8) |
            c8->memory[c8->program_counter+1];

#ifdef DEBUG
        printf("Memory[0x%03x]: 0x%04x\n", c8->program_counter, instruction);
#endif

        // Advance program counter
        c8->program_counter += 2;

        // Decode/Execute
        (g_execute[(instruction & 0xf000) >> 12])(c8, instruction);
    }
}

void *chip8_fn(__attribute__ ((unused)) void *p)
{
    // Init
    chip8_t c8 = {0};
    c8.program_counter = PROGRAM_START;
    c8.stack_pointer = -1;
    srand(time(NULL));

    load(&c8);

#ifdef DEBUG
    print_memory(&c8);
#endif

    while (!g_timer_start);

    //run(&c8);

    pthread_exit(NULL);
}
