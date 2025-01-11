#ifndef DEBUGGER_H_
#define DEBUGGER_H_

#include <stdbool.h>

/**
 * Turn on debugging mode
 * @since 0.1.0
 */
void set_debug();

/**
 * Return the state of debugging
 * @return true if debugging is turned on, false otherwise
 * @since 0.1.0
 */
bool should_debug();

/**
 * Print to screen (like `printf()`) if debugging is turned on
 * @param format_string: same as for `printf()`
 * @since 0.1.0
 */
void debug_printf(const char *format_string, ...)
    __attribute__((format(printf, 1, 2)));


/**
 * Prints the state of registers
 * @param regs: registers to print
 * @since 0.1.0
 */
void print_registers(unsigned char *regs);

/**
 * Prints the state of the stack
 * @param stack: start pointer of the stack
 * @param stack_size: size of the stack
 * @param sp: the stack pointer
 * @since 0.1.0
 */
void print_stack(unsigned char *stack, unsigned char stack_size,
                 unsigned char sp);

/**
 * Prints the state of the memory
 * @param memory: pointer to the chip8 RAM
 * @param pc: the program counter
 * @since 0.1.0
 */
void print_memory(unsigned char *memory, unsigned short pc);

/**
 * Set the error message to be printed by `print_error()`
 * @param new_err_msg: error message to print out
 * @since 0.1.0
 */
void set_error(const char *new_err_msg);

/**
 * Prints the error message set by `set_error()`
 * @since 0.1.0
 */
void print_error();

#endif
