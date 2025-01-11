#ifndef GRAPHICS_H_
#define GRAPHICS_H_

#include <stdbool.h>

/**
 * Runs all the ncurses initialization routines
 * @since 0.1.0
 */
void init_graphics();

/**
 * Draws the changed part of the video buffer
 * @param video_mem: chip8 video buffer
 * @param video_signal: return signal that can be decoded like this:
 *  - first digit is ALWAYS DRAW or DRAW_HI_RES Flag
 *  - second digit is the number of rows of a sprite
 *  - digits at positions 0x00ffff00 encode the x and y positions of the sprite
 *    like so: y * NUM_BYTES_IN_ROW + x
 * @param hi_res: is the high resolution mode on
 * @since 0.1.0
 */
void draw(unsigned char *video_mem, unsigned int video_signal, bool hi_res);

/**
 * Clears the video display
 * @since 0.1.0
 */
void clear_screen();

/**
 * Sets the outer parts on or off
 * @param is_pixel_on: if true, turn on the outer part of display
 * @since 0.1.0
 */
void st_flash(bool is_pixel_on);

/**
 * Draw the xset message, wait for some time, then clear it
 * @since 0.1.0
 */
void handle_xset_message();

/**
 * Draws the entire video buffer
 * @param video_mem: chip8 video buffer
 * @param hi_res: is the high resolution mode on
 * @since 0.1.0
 */
void draw_all(unsigned char *video_mem, bool hi_res);

/**
 * Displays a message if the screen is too small
 * @param video_mem: chip8 video buffer (used for redrawing)
 * @param hi_res: is the high resolution mode on (used for redrawing)
 * @since 0.1.0
 */
void handle_win_size(unsigned char *video_mem, bool hi_res);

#endif
