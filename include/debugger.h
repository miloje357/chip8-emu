#ifndef DEBUGGER_H_
#define DEBUGGER_H_

#include <stdbool.h>
void set_debug();
void debug_printf(const char *format_string, ...)
    __attribute__((format(printf, 1, 2)));
bool should_debug();

#endif
