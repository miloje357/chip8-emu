#ifndef CHIP8_H_
#define CHIP8_H_

#include <stdbool.h>
#include <stdio.h>

/**
 * Memory layout constants
 * @since 1.2.0
 */
#define STACK_START 0xee0
#define STACK_END 0xf00
#define PROGRAM_START 0x200
#define START_VIDEO_MEM 0xf00
#define SIZE_MEMORY (START_VIDEO_MEM + SIZE_VIDEO_MEM)

/**
 * Screen dimention constants
 * @since 0.1.0
 */
#define WIDTH 128
#define HEIGTH 64
#define SIZE_VIDEO_MEM (WIDTH * HEIGTH) / 8

/**
 * Macros for manipulating the signal for drawing
 * @since 0.1.0
 */
#define GET_N(sig) (sig & 0x000000f0) >> 4
#define GET_XY(sig) (sig & 0x00ffff00) >> 8
#define SET_N(n) (n) << 4
#define SET_XY(xy) (xy) << 8

/**
 * Everything that chip8 cares about
 * @since 1.2.0
 */
typedef struct {
    unsigned char V[16];               /**< Registers from V0 to VF */
    unsigned short pc;                 /**< Program counter */
    unsigned char sp;                  /**< Stack pointer */
    unsigned short I;                  /**< Index register */
    unsigned char dt, st;              /**< Delay and sound timer */
    bool hi_res;                       /**< HIgh resolution*/
    bool has_superchip8_quirks;        /**< SuperChip8 quirks*/
    unsigned char flags[16];           /**< Flag registers */
    unsigned char memory[SIZE_MEMORY]; /**< RAM and VRAM */
} Chip8Context;

/**
 * Flags for IO control
 * @since 0.1.0
 */
typedef enum {
    IDLE_NOT_EXECUTED, /**< Flag for doing nothing and instruction not executed
                        */
    IDLE_EXECUTED,     /**< Flag for doing nothing and instruction executed */
    DRAW,              /**< Flag for drawing to screen */
    DRAW_HI_RES, /**< Flag for drawing to screen in high resoultion mode */
    CLEAR,       /**< Flag for clearing the screen */
    SCROLL,      /**< Flag for scrolling the screen */
    SOUND,       /**< Flag for FLASHING the screen (not buzzing the buzzer)*/
    KEYBOARD_BLOCKING, /**< Flag for getting keyboard input. Waits until input.
                        */
    KEYBOARD_NONBLOCKING, /**< Flag for getting keyboard input. Doesn't wait
                             until input. */
    EXIT,                 /**< Flag for shutdown */
} Flag;

/**
 * Loads program to memory
 * @param program: FILE handle of the program
 * @return 0 if everything is ok, 1 otherwise
 * @since 1.2.0
 */
int load_program(FILE* program_path);

/**
 * Initializes the program counter and stack pointer
 * @since 0.1.0
 */
void init_chip8();

/**
 * Executes the next step in the fetch-decode-execute cycle
 * @return signal that can be decoded like this:
 *  - first digit is always the Flag
 *  - other digits ONLY concern the DRAW and DRAW_HI_RES Flags
 *      - second digit is the number of rows of a sprite
 *      - digits at positions 0x00ffff00 encode the x and y positions of the
 *        sprite like so: y * NUM_BYTES_IN_ROW + x
 * @since 0.1.0
 */
unsigned int next_cycle();

/**
 * Get pointer to the video buffer memory
 * @return pointer to the video buffer
 * @since 0.1.0
 */
unsigned char* get_video_mem();

/**
 * Decrement the sound and delay timers
 * @return SOUND if the sound delay goes to 0, IDLE otherwise
 * @since 0.1.0
 */
Flag decrement_timers();

#define KEYBOARD_UNSET 0xff
/**
 * Checks the key and performs the SKP or SKNP
 * @param reg MUST SET TO KEYBOARD_UNSET
 * @param is_equal MUST SET TO KEYBOARD_UNSET
 * @param key key to check
 * @since 0.1.0
 */
void skip_key(unsigned char reg, bool is_equal, unsigned char key);

/**
 * Checks the key and performs the LD K, Vx instruction
 * @param reg MUST SET TO KEYBOARD_UNSET
 * @param key key to check
 * @since 0.1.0
 */
void load_key(unsigned char reg, unsigned char key);

/**
 * Sets the system to use super chip8 quirks
 * @since 0.1.0
 */
void set_superchip8_quirks();

/**
 * Gets hi_res
 * @return true if runs in high resolution mode, false otherwise
 * @since 0.1.0
 */
bool get_hi_res();

/**
 * Gets current Chip8Context
 * @return Chip8Context
 * @since 1.2.0
 */
Chip8Context* get_chip8();

#endif
