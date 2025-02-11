#ifndef ASM_VIEW_H_
#define ASM_VIEW_H_

/**
 * @see scroll_by
 * @since 1.2.0
 */
#include <stdbool.h>
#include <stdio.h>
typedef enum { LINE, LABEL, TOP, BOTTOM } ScrollUnit;

/**
 * Set the geometry of the assembly view
 * @param y: y coordinate of the origin of the assembly view
 * @param x: x coordinate of the origin of the assembly view
 * @param h: height of the assembly view
 * @param w: width of the assembly view
 * @since 1.2.0
 */
void set_assembly_dimens(int y, int x, int h, int w);

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

// TODO: Write docs
void init_assembly_graphics();
void delete_assembly_graphics();

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
