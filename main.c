
#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "chip8.h"
#include "display.h"
#include "io.h"
#include "load.h"
#include "timer.h"

static void flush_stdin()
{
    char x;
    while((x = getchar()) != '\n' && x != EOF); // discard
}

static int get_lf_index(char buf[], int n)
{
    for (int i = (n-1); i > -1; i--)
    {
        if (buf[i] == '\n')
            return i;
    }
    return -1;
}

static int is_hex(char buf[], const size_t n, const int lf_index)
{
    size_t i = 0;

    if (lf_index > 6)
    {
        if ((buf[0] == '0') && (buf[1] == 'x'))
            i += 2;
        else
            return 0;
    }

    if ((buf[0] == '0') && (buf[1] == 'x'))
        i += 2;

    for (; i < n; i++)
    {
        if (buf[i] == '\0')
            continue;
        if (!isxdigit(buf[i]))
            return 0;
    }

    return 1;
}

static const char *INVALID_HEX_CODE = "Invalid hex code. Try again!";

static void change_color(const char *which, uint32_t *color_address)
{
    char color_str[10] = {0};
    const size_t COLOR_SIZE = sizeof(color_str);
    int lf_index;
    uint32_t color;
    while (1)
    {
        printf("%s > ", which);

        if (
            (fgets(color_str, COLOR_SIZE, stdin) == NULL) ||
            ((lf_index = get_lf_index(color_str, COLOR_SIZE)) < 0)
        )
        {
            printf("%s\n", INVALID_HEX_CODE);
            flush_stdin();
            continue;
        }

        if (lf_index == 0)
        {
            printf(
                "%s color remains #%06X\n",
                which,
                (*color_address & 0x00ffffff)
            );
            break;
        }

        color_str[lf_index] = '\0';
        if (!is_hex(color_str, COLOR_SIZE, lf_index))
        {
            printf("%s\n", INVALID_HEX_CODE);
            continue;
        }

        color = (0x00ffffff & strtoul(color_str, NULL, 16));
        *color_address &= 0xff000000;
        *color_address |= color;
        printf("%s color set to #%06X\n", which, color);
        break;
    }
}

static void enter_color_prompt()
{
    printf("Enter 'y' for colors: ");

    char c = getchar();
    if ((c == 'y') || (c == 'Y'))
    {
        flush_stdin();
        change_color("Background", &g_background_color);
        change_color("Foreground", &g_foreground_color);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("[USAGE] %s ROM\n", argv[0]);
        return 1;
    }
    else
    {
        g_romfile = argv[1];
    }

    enter_color_prompt();

    pthread_t t1, t2;
    io_init();
    pthread_mutex_init(&g_display_mutex, NULL);
    pthread_mutex_init(&g_input_mutex, NULL);
    pthread_mutex_init(&g_timer_mutex, NULL);
    pthread_cond_init(&g_input_cond, NULL);
    pthread_create(&t1, NULL, timer_fn, NULL);
    pthread_create(&t2, NULL, cpu_fn, NULL);
    io_loop();
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_cond_destroy(&g_input_cond);
    pthread_mutex_destroy(&g_display_mutex);
    pthread_mutex_destroy(&g_input_mutex);
    pthread_mutex_destroy(&g_timer_mutex);
    io_quit();
    return 0;
}
