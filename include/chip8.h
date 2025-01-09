#ifndef CHIP8_H_
#define CHIP8_H_

#include <stdbool.h>
#define KEYBOARD_UNSET 0xff
#define PROGRAM_START 0x200
#define START_VIDEO_MEM 0xf00
#define WIDTH 128
#define HEIGTH 64
#define SIZE_VIDEO_MEM (WIDTH * HEIGTH) / 8

#define GET_N(sig) (sig & 0x000000f0) >> 4
#define GET_XY(sig) (sig & 0x00ffff00) >> 8
#define GET_HI_RES(sig) (sig & 0x0f000000) >> 24

#define SET_N(n) (n) << 4
#define SET_XY(xy) (xy) << 8
#define SET_HI_RES(hi_res) (hi_res) << 24

typedef enum {
    IDLE,
    DRAW,
    DRAW_HI_RES,
    CLEAR,
    SCROLL,
    SOUND,
    KEYBOARD_BLOCKING,
    KEYBOARD_NONBLOCKING,
    EXIT,
} Flag;

int load_program(const char* program_path);
void print_state();
void init_chip8();
unsigned int next_cycle();
unsigned char* get_video_mem();
Flag decrement_timers();
void skip_key(unsigned char reg, bool is_equal, unsigned char key);
void load_key(unsigned char reg, unsigned char key);
void set_superchip8_quirks();

#endif
