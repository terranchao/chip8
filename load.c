/*
 * This file contains the code that is responsible for loading the font graphic
 * data and the CHIP-8 program instructions into interpreter memory. This is run
 * once at the beginning of the CPU thread.
 */
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chip8.h"
#include "load.h"

char *g_romfile = NULL;

const uint16_t PROGRAM_START = 0x200;
static const unsigned long MAX_PROGRAM_SIZE = (MEMORY_SIZE-PROGRAM_START);

#ifdef COSMAC_VIP
const size_t FONT_START = 0x000;
#else
const size_t FONT_START = 0x050;
#endif
static const uint8_t g_font[] =
{
    0xf0, 0x90, 0x90, 0x90, 0xf0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xf0, 0x10, 0xf0, 0x80, 0xf0, // 2
    0xf0, 0x10, 0xf0, 0x10, 0xf0, // 3
    0x90, 0x90, 0xf0, 0x10, 0x10, // 4
    0xf0, 0x80, 0xf0, 0x10, 0xf0, // 5
    0xf0, 0x80, 0xf0, 0x90, 0xf0, // 6
    0xf0, 0x10, 0x20, 0x40, 0x40, // 7
    0xf0, 0x90, 0xf0, 0x90, 0xf0, // 8
    0xf0, 0x90, 0xf0, 0x10, 0xf0, // 9
    0xf0, 0x90, 0xf0, 0x90, 0x90, // A
    0xe0, 0x90, 0xe0, 0x90, 0xe0, // B
    0xf0, 0x80, 0x80, 0x80, 0xf0, // C
    0xe0, 0x90, 0x90, 0x90, 0xe0, // D
    0xf0, 0x80, 0xf0, 0x80, 0xf0, // E
    0xf0, 0x80, 0xf0, 0x80, 0x80, // F
};
const size_t FONT_SIZE = (sizeof(g_font)/16);

static void handle_fatal_error(FILE *fp)
{
    if (fp) fclose(fp);
    pthread_exit(NULL);
}

void load_memory(uint8_t *memory)
{
    printf("File: %s\n", g_romfile);

    // Open file
    FILE *fp = fopen(g_romfile, "rb");
    if (!fp)
    {
        printf("[ERROR] Unable to open file\n");
        handle_fatal_error(fp);
    }

    // Check file size
    fseek(fp, 0, SEEK_END);
    const unsigned long file_size = ftell(fp);
    if (file_size < 2)
    {
        printf("[ERROR] Size: %lu\n", file_size);
        handle_fatal_error(fp);
    }
    else if (file_size > MAX_PROGRAM_SIZE)
    {
        printf(
            "[ERROR] Size exceeds maximum of %lu bytes\n", MAX_PROGRAM_SIZE
        );
        handle_fatal_error(fp);
    }
    printf("Size: %lu bytes\n", file_size);

    // Rewind and read into memory
    rewind(fp);
    if (fread(&memory[PROGRAM_START], 1, file_size, fp))
    {
        printf("Loaded into memory address 0x%03x successfully\n",
            PROGRAM_START);
        fclose(fp);
    }
    else
    {
        printf("[ERROR] Read failed\n");
        handle_fatal_error(fp);
    }

    // Load font
    memcpy(&memory[FONT_START], g_font, sizeof(g_font));
}

#ifdef DEBUG
void print_memory(const uint8_t *memory)
{
    printf("%s:", __func__);
    for (size_t i = 0; i < MEMORY_SIZE; i++)
    {
        if ((i % 16) == 0)
        {
            printf("\n%03lx  ", i);
        }
        else if ((i % 8) == 0)
        {
            printf(" ");
        }
        printf("%02x ", memory[i]);
    }
    printf("\n");
}
#endif
