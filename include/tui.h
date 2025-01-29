#ifndef TUI_H_
#define TUI_H_

#include <stdbool.h>

/**
 * Draw a border at (y, x) of height h and width w
 * @param y: the y coordinate of the border
 * @param x: the x coordinate of the border
 * @param h: the height of the border
 * @param w: the width of the border
 * @since 1.2.0
 */
void draw_border(int y, int x, int h, int w);

/**
 * Clear an area at (y, x) of height h and width w
 * @param y: the y coordinate of the top-left vertex
 * @param x: the x coordinate of the top-left vertex
 * @param h: the height of the area
 * @param w: the width of the area
 * @since 1.2.0
 */
void clear_area(int y, int x, int h, int w);

/**
 * Runs all the ncurses initialization routines and game/debug init routines
 * @param has_debugging: true if the emulator runs in graphical debugging mode
 * @since 1.2.0
 */
void init_graphics(bool has_debugging);

/**
 * Displays a message if the screen is too small
 * @param video_mem: chip8 video buffer (used for redrawing)
 * @param hi_res: is the high resolution mode on (used for redrawing)
 * @since 1.2.0
 */
void handle_win_size(unsigned char *video_mem, bool hi_res);

/**
 * Runs game/debug init routines and redraws everything
 * @param video_mem: chip8 video buffer (used for redrawing)
 * @param hi_res: is the high resolution mode on (used for redrawing)
 * @param has_debugging: true if the emulator runs in graphical debugging mode
 * @since 1.2.0
 */
void reset_graphics(unsigned char *video_mem, bool hi_res, bool has_debugging);

#endif
