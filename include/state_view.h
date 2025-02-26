#ifndef STATE_VIEW_H_
#define STATE_VIEW_H_

#include <stdbool.h>

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
 * Set the geometry of the state view
 * @param y: y coordinate of the origin of the state view
 * @param x: x coordinate of the origin of the state view
 * @param h: height of the state view
 * @param w: width of the state view
 * @since 1.2.0
 */
void set_state_dimens(int y, int x, int h, int w);

/**
 * Initializes the state view window and runs needed ncurses routines
 * @since 1.2.0
 */
void init_state_graphics();

/**
 * Frees the state view window
 * @since 1.2.0
 */
void delete_state_graphics();

/**
 * Draws registers, program counter, stack pointer, index, delay and sound
 * timers
 * @see `print_state()`
 * @param chip8: pointer to a Chip8Context (see `get_chip8()` in
 * include/chip8.h)
 * @since 1.2.0
 */
void draw_state(Chip8Context *chip8);

/**
 * Displays a message in the message view
 * @param message: message to be displayed
 * @since 1.2.0
 */
void set_message(const char *message);

/**
 * Clear the message view
 * @since 1.2.0
 */
void clear_message_view();

/**
 * Displays an input field in state view
 * @param title: title displayed above the input field
 * @param len: the length of the returned string
 * @return: a string entered in the input field
 * @note: the returned string must be freed
 * @since 1.2.0
 */
char *get_from_field(const char *title, size_t *len);

#endif
