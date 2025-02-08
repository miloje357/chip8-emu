#ifndef DEBUGGER_H_
#define DEBUGGER_H_

#include <stdbool.h>
#include <stdio.h>

#include "chip8.h"

/**
 * All the states of debugging
 * @since 1.2.0
 */
typedef enum {
    NO_DEBUGGING,      /*>> Emulator runs normally*/
    GRAPHIC_DEBUGGING, /*>> Emulator displays the graphics and some debugging
                          information*/
    CONSOLE_DEBUGGING  /*>> Emulator doesn't display graphics, only the current
                          state*/
} DebugType;

/**
 * @see scroll_by
 * @since 1.2.0
 */
typedef enum { LINE, LABEL, TOP, BOTTOM } ScrollUnit;

/*
 * Set the debugging mode
 * @param type: proper DebugType
 * @since 1.2.0
 */
void set_debugging(DebugType type);

/**
 * Return the state of debugging
 * @return proper DebugType
 * @since 1.2.0
 */
DebugType get_debugging();

/**
 * Print to screen (like `printf()`) if debugging is turned on
 * @param format_string: same as for `printf()`
 * @since 0.1.0
 */
void debug_printf(const char *format_string, ...)
    __attribute__((format(printf, 1, 2)));

/**
 * Prints registers, program counter, stack pointer, index, video buffer and
 * some memory
 * @see `print_register()`
 * @param chip8: pointer to a Chip8Context (see get_chip8 in include/chip8.h)
 * @since 1.2.0
 */
void print_state(Chip8Context *chip8);

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

/**
 * Set the geometry of the debugger
 * @param y: y coordinate of the origin of the debugger
 * @param x: x coordinate of the origin of the debugger
 * @param h: height of the debugger
 * @param w: width of the debugger
 * @since 1.2.0
 */
void set_debug_dimes(int y, int x, int h, int w);

/**
 * Initializes the debugger window and runs needed ncurses routines
 * @since 1.2.0
 */
void init_debug_graphics();

/**
 * Frees the debugger window
 * @since 1.2.0
 */
void delete_debug_graphics();

/**
 * Disassembles the current program and loads it to memory
 * @param src: The source program
 * @param has_quirks: true if the emulator runs with Super Chip8 quirks
 * @since 1.2.0
 */
void set_assembly(FILE *src, bool has_quirks);

/**
 * Frees the assembly memory
 * @since 1.2.0
 */
void free_assembly();

/**
 * Selects the current instruction in assembly view
 * @param pc: the program counter
 * @since 1.2.0
 */
void set_curr_inst(unsigned short pc);

/**
 * Scroll by some number of scroll units
 * @param unit: see ScrollUnit
 * @param num: number of units to scroll (ignored in case of TOP or BOTTOM)
 * @since 1.2.0
 */
void scroll_by(ScrollUnit unit, int num);

#endif
