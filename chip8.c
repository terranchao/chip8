/*
 * The functions in this file implement the CHIP-8 CPU thread, including the
 * entire CHIP-8 instruction set.
 */
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "chip8.h"
#include "draw.h"
#include "io.h"
#include "load.h"
#include "terminal.h"
#include "timer.h"

volatile uint8_t g_cpu_done = 0;
volatile uint8_t g_in_fx0a = 0;

static const int8_t MAX_STACK_INDEX = (STACK_SIZE-1);

static const char *DEST_ADDR_OOR = "Destination address is out of range";

static void handle_error(
    const char *message, const uint16_t bad_address, const uint16_t instruction
)
{
    printf(
        "[ERROR] %s (Memory[0x%03x]: 0x%04x)\n",
        message, bad_address, instruction
    );
    g_cpu_done = 1;
    pthread_exit(NULL);
}

static void undefined_instruction(chip8_t *c8, const uint16_t instruction)
{
    handle_error(
        "Encountered undefined instruction",
        c8->program_counter-2, instruction
    );
}

static inline void advance_program_counter(chip8_t *c8)
{
    c8->program_counter += 2;
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
    c8->program_counter = address;
}

static void execute_2nnn(chip8_t *c8, const uint16_t instruction)
{
    // Call subroutine
    if (c8->stack_pointer == MAX_STACK_INDEX)
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
    c8->stack_pointer++;
    c8->stack[c8->stack_pointer] = c8->program_counter;
    c8->program_counter = address;
}

static void execute_3xnn(chip8_t *c8, const uint16_t instruction)
{
    // Skip next instruction if Vx == byte
    if (c8->V[(instruction & 0x0f00) >> 8] == (instruction & 0x00ff))
    {
        advance_program_counter(c8);
    }
}

static void execute_4xnn(chip8_t *c8, const uint16_t instruction)
{
    // Skip next instruction if Vx != byte
    if (c8->V[(instruction & 0x0f00) >> 8] != (instruction & 0x00ff))
    {
        advance_program_counter(c8);
    }
}

static void execute_5xy0(chip8_t *c8, const uint16_t instruction)
{
    // Skip next instruction if Vx == Vy
    if (
        c8->V[(instruction & 0x0f00) >> 8] == c8->V[(instruction & 0x00f0) >> 4]
    )
    {
        advance_program_counter(c8);
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
    c8->V[0xf] = 0x00;
}

static void execute_8xy2(chip8_t *c8, const uint16_t instruction)
{
    // Vx &= Vy
    c8->V[(instruction & 0x0f00) >> 8] &= c8->V[(instruction & 0x00f0) >> 4];
    c8->V[0xf] = 0x00;
}

static void execute_8xy3(chip8_t *c8, const uint16_t instruction)
{
    // Vx ^= Vy
    c8->V[(instruction & 0x0f00) >> 8] ^= c8->V[(instruction & 0x00f0) >> 4];
    c8->V[0xf] = 0x00;
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
    const uint8_t before = c8->V[x];
    c8->V[x] -= c8->V[(instruction & 0x00f0) >> 4];
    c8->V[0xf] = (c8->V[x] > before) ? 0 : 1;
}

static void execute_8xy6(chip8_t *c8, const uint16_t instruction)
{
#ifdef COSMAC_VIP
    // Vx = (Vy >>= 1)
    const uint8_t y = ((instruction & 0x00f0) >> 4);
    const uint8_t flag = (c8->V[y] & 0x01);
    c8->V[y] >>= 1;
    c8->V[(instruction & 0x0f00) >> 8] = c8->V[y];
    c8->V[0xf] = flag;
#else
    // Vx >>= 1
    const uint8_t x = ((instruction & 0x0f00) >> 8);
    const uint8_t flag = (c8->V[x] & 0x01);
    c8->V[x] >>= 1;
    c8->V[0xf] = flag;
#endif
}

static void execute_8xy7(chip8_t *c8, const uint16_t instruction)
{
    // Vx = (Vy - Vx)
    const uint8_t x = ((instruction & 0x0f00) >> 8);
    const uint8_t y = ((instruction & 0x00f0) >> 4);
    c8->V[x] = (c8->V[y] - c8->V[x]);
    c8->V[0xf] = (c8->V[x] > c8->V[y]) ? 0 : 1;
}

static void execute_8xye(chip8_t *c8, const uint16_t instruction)
{
#ifdef COSMAC_VIP
    // Vx = (Vy <<= 1)
    const uint8_t y = ((instruction & 0x00f0) >> 4);
    const uint8_t flag = ((c8->V[y] & 0x80) >> 7);
    c8->V[y] <<= 1;
    c8->V[(instruction & 0x0f00) >> 8] = c8->V[y];
    c8->V[0xf] = flag;
#else
    // Vx <<= 1
    const uint8_t x = ((instruction & 0x0f00) >> 8);
    const uint8_t flag = ((c8->V[x] & 0x80) >> 7);
    c8->V[x] <<= 1;
    c8->V[0xf] = flag;
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
        advance_program_counter(c8);
    }
}

static void execute_annn(chip8_t *c8, const uint16_t instruction)
{
    // Set I
    c8->I = (instruction & 0x0fff);
}

static void execute_bnnn(chip8_t *c8, const uint16_t instruction)
{
#ifdef COSMAC_VIP
    // Jump to address + V0
    const uint16_t address = c8->V[0x0] + (instruction & 0x0fff);
#else
    // Jump to address + Vx
    const uint16_t address =
        c8->V[(instruction & 0x0f00) >> 8] + (instruction & 0x0fff);
#endif
    if ((address < PROGRAM_START) || (address >= MEMORY_SIZE))
    {
        handle_error(DEST_ADDR_OOR, c8->program_counter-2, instruction);
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
    if (g_keystate[g_keymap[c8->V[(instruction & 0x0f00) >> 8]]])
    {
        advance_program_counter(c8);
    }
}

static void execute_exa1(chip8_t *c8, const uint16_t instruction)
{
    // Skip next instruction if key in Vx is not pressed
    if ((instruction & 0x00f0) != 0x00a0)
    {
        undefined_instruction(c8, instruction);
    }
    if (!g_keystate[g_keymap[c8->V[(instruction & 0x0f00) >> 8]]])
    {
        advance_program_counter(c8);
    }
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
    // Vx = delay timer
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
    // Wait for key press
    if ((instruction & 0x00f0) != 0x0000)
    {
        undefined_instruction(c8, instruction);
    }
    pthread_mutex_lock(&g_input_mutex);
    if (!(g_io_done || g_restart || g_pause))
    {
        g_in_fx0a = 1;
        pthread_cond_wait(&g_input_cond, &g_input_mutex);
        g_in_fx0a = 0;
        c8->V[(instruction & 0x0f00) >> 8] = g_key_released;
    }
    pthread_mutex_unlock(&g_input_mutex);
}

static void execute_fx15(chip8_t *c8, const uint16_t instruction)
{
    // Delay timer = Vx
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
    // Sound timer = Vx
    if ((instruction & 0x00f0) != 0x0010)
    {
        undefined_instruction(c8, instruction);
    }
    const uint8_t duration = c8->V[(instruction & 0x0f00) >> 8];
    if (duration < 0x02) return;
    pthread_mutex_lock(&g_timer_mutex);
    g_sound_timer = duration;
    pthread_mutex_unlock(&g_timer_mutex);
}

static void execute_fx1e(chip8_t *c8, const uint16_t instruction)
{
    // I += Vx
    if ((instruction & 0x00f0) != 0x0010)
    {
        undefined_instruction(c8, instruction);
    }
    c8->I += c8->V[(instruction & 0x0f00) >> 8];
}

static void execute_fx29(chip8_t *c8, const uint16_t instruction)
{
    // I = sprite address
    if ((instruction & 0x00f0) != 0x0020)
    {
        undefined_instruction(c8, instruction);
    }
    c8->I =
        FONT_START +
        FONT_SIZE*(c8->V[(instruction & 0x0f00) >> 8] & 0x0f);
}

static void execute_fx33(chip8_t *c8, const uint16_t instruction)
{
    // Store Vx in binary-coded decimal
    if ((instruction & 0x00f0) != 0x0030)
    {
        undefined_instruction(c8, instruction);
    }
    uint8_t x = c8->V[(instruction & 0x0f00) >> 8];
    c8->memory[c8->I+2] = (x % 10);
    x /= 10;
    c8->memory[c8->I+1] = (x % 10);
    x /= 10;
    c8->memory[c8->I] = (x % 10);
}

static void execute_fx55(chip8_t *c8, const uint16_t instruction)
{
    // Store registers
    if ((instruction & 0x00f0) != 0x0050)
    {
        undefined_instruction(c8, instruction);
    }
    const uint16_t num_registers = (((instruction & 0x0f00) >> 8) + 1);
    memcpy(&c8->memory[c8->I], c8->V, num_registers);
    c8->I += num_registers;
}

static void execute_fx65(chip8_t *c8, const uint16_t instruction)
{
    // Load registers
    if ((instruction & 0x00f0) != 0x0060)
    {
        undefined_instruction(c8, instruction);
    }
    const uint16_t num_registers = (((instruction & 0x0f00) >> 8) + 1);
    memcpy(c8->V, &c8->memory[c8->I], num_registers);
    c8->I += num_registers;
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

static void reset(chip8_t *c8)
{
    memset(c8, 0, sizeof(*c8));
    c8->program_counter = PROGRAM_START;
    c8->stack_pointer = -1;

    load_memory(c8->memory);

#ifdef DEBUG
    print_memory(c8->memory);
#endif
}

static void process_ui_controls(chip8_t *c8, const uint16_t instruction)
{
    uint8_t in_restart = 0;
    uint8_t in_pause = 0;

    while (!g_io_done)
    {
        if (g_restart)
        {
            draw_restart_icon();
            in_restart ^= 1;
            g_restart = 0;
        }

        if (g_pause)
        {
            if (in_pause) continue;

            draw_pause_icon();
            if ((instruction & 0xf0ff) == 0xf00a)
            {
                // If a pause interrupts a wait for a keypress, redo it.
                c8->program_counter -= 2;
            }
            in_pause = 1;
        }
        else
        {
            if (in_restart)
            {
                reset(c8);
                clear_display();
                clear_terminal();
            }
            else if (in_pause)
            {
                draw_pause_icon();
            }
            break;
        }
    }
}

static void run(chip8_t *c8)
{
#ifdef DEBUG
    printf("%s start\n", __func__);
#endif
    while (!g_io_done)
    {
        // Fetch
        const uint16_t instruction =
            (c8->memory[c8->program_counter] << 8) |
            c8->memory[c8->program_counter+1];

        write_registers_to_terminal(c8, instruction);

        advance_program_counter(c8);

        // Decode/Execute
        (g_execute[(instruction & 0xf000) >> 12])(c8, instruction);

        process_ui_controls(c8, instruction);
    }
}

void *cpu_fn(__attribute__ ((unused)) void *p)
{
    chip8_t c8;
    reset(&c8);

    srand(time(NULL));

    while (!g_timer_start);

    clear_display();

    init_terminal();

    run(&c8);

    quit_terminal();

#ifdef DEBUG
    printf("%s exit\n", __func__);
#endif

    g_cpu_done = 1;
    pthread_exit(NULL);
}
