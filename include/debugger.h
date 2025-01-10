#ifndef DEBUGGER_H_
#define DEBUGGER_H_

#include <stdbool.h>
void set_debug();
void debug_printf(const char *format_string, ...)
    __attribute__((format(printf, 1, 2)));
bool should_debug();
void print_registers(unsigned char *regs);
void print_stack(unsigned char *stack, unsigned char stack_size,
                 unsigned char sp);
void print_memory(unsigned char *memory, unsigned short pc);
void set_error(const char *new_err_msg);
void print_error();

#endif
