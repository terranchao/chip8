
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chip8.h"
#include "load.h"

char *g_romfile = NULL;

const uint16_t PROGRAM_START = 0x200;
static const unsigned long MAX_PROGRAM_SIZE = MEMORY_SIZE - PROGRAM_START;

const size_t FONT_START = 0x050;

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

static inline void handle_fatal_error(FILE *fp)
{
    if (fp) fclose(fp);
    exit(EXIT_FAILURE);
}

void load(chip8_t *c8)
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
    if (fread(&c8->memory[PROGRAM_START], 1, file_size, fp))
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
    memcpy(&c8->memory[FONT_START], g_font, sizeof(g_font));
}

#ifdef DEBUG
void print_memory(const chip8_t *c8)
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
        printf("%02x ", c8->memory[i]);
    }
    printf("\n");
}
#endif
