#include <ncurses.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

bool debug = false;

void set_debug() { debug = true; }

bool should_debug() { return debug; }

void debug_printf(const char *format, ...) {
    if (!debug) {
        /* char *str;
        va_list args;
        va_start(args, format);
        vasprintf(&str, format, args);
        va_end(args);
        mvprintw(0, 0, "%s", str); */
        return;
    }
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
