#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

bool debug = false;

void set_debug() {
	debug = true;
}

bool should_debug() {
	return debug;
}

void debug_printf(const char *format, ...) {
	if (!debug) {
		return;
	}
	va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
}
