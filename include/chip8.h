#ifndef CHIP8_H_
#define CHIP8_H_

#include <stdbool.h>
#define KEYBOARD_UNSET 0xff
#define PROGRAM_START 0x200
#define START_VIDEO_MEM 0xf00

typedef enum {
    IDLE,
    DRAW,
    SOUND,
    KEYBOARD_BLOCKING,
    KEYBOARD_NONBLOCKING,
} Flag;

int load_program(const char* program_path);
void print_state();
void init_chip8();
Flag next_cycle();
unsigned char* get_video_mem();
Flag decrement_timers();
void skip_key(unsigned char reg, bool is_equal, unsigned char key);
void load_key(unsigned char reg, unsigned char key);

#endif
