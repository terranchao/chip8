/*
 * This file contains code that updates the terminal screen with the CHIP-8
 * register values. This code belongs to the CPU thread.
 */
#include <ncurses.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "chip8.h"
#include "terminal.h"
#include "timer.h"

#define NUM_ROWS_OF_OUTPUT 11

static int g_terminal_rows[NUM_ROWS_OF_OUTPUT] = {0};

void init_terminal()
{
    initscr();
    curs_set(0); // hide cursor
    int terminal_height = getmaxy(stdscr);
    for (size_t i = 0; i < NUM_ROWS_OF_OUTPUT; i++)
    {
        g_terminal_rows[i] = (terminal_height-NUM_ROWS_OF_OUTPUT+i);
    }
}

void write_registers_to_terminal(const chip8_t *c8, const uint16_t instruction)
{
    mvprintw(
        g_terminal_rows[0], 0,
        "Address %03x  Instruction %04x",
        c8->program_counter, instruction
    );

    mvprintw(g_terminal_rows[2], 0, "Timers");
    pthread_mutex_lock(&g_timer_mutex);
    mvprintw(
        g_terminal_rows[3], 0,
        "Delay %02x  Sound %02x",
        g_delay_timer, g_sound_timer
    );
    pthread_mutex_unlock(&g_timer_mutex);

    mvprintw(
        g_terminal_rows[5], 0,
        "V   0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F"
    );
    mvprintw(
        g_terminal_rows[6], 0,
        "   "
        "%02x  %02x  %02x  %02x  "
        "%02x  %02x  %02x  %02x  "
        "%02x  %02x  %02x  %02x  "
        "%02x  %02x  %02x  %02x  ",
        c8->V[0x0], c8->V[0x1], c8->V[0x2], c8->V[0x3],
        c8->V[0x4], c8->V[0x5], c8->V[0x6], c8->V[0x7],
        c8->V[0x8], c8->V[0x9], c8->V[0xa], c8->V[0xb],
        c8->V[0xc], c8->V[0xd], c8->V[0xe], c8->V[0xf]
    );

    mvprintw(g_terminal_rows[8], 0, "Stack");
    char stack_str[4*STACK_SIZE] = {0};
    for (size_t i = 0; i < STACK_SIZE; i++)
    {
        sprintf(stack_str+i*4, "%03x ", c8->stack[i]);
    }
    mvprintw(g_terminal_rows[9], 0, "%s", stack_str);

    char stack_pointer_str[4*STACK_SIZE+1];
    stack_pointer_str[sizeof(stack_pointer_str)-1] = '\0';
    memset(stack_pointer_str, ' ', sizeof(stack_pointer_str)-1);
    if (c8->stack_pointer > -1)
    {
        stack_pointer_str[4*c8->stack_pointer] = '*';
    }
    mvprintw(g_terminal_rows[10], 0, "%s", stack_pointer_str);

    refresh();
}

void clear_terminal()
{
    clear();
}

void quit_terminal()
{
    endwin();
}
