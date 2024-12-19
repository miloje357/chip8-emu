#ifndef CHIP8_H_
#define CHIP8_H_

typedef enum {
    IDLE,
    DRAW
} Flag;

int load_program(const char* program_path);
void print_state();
void init_chip8();
Flag next_cycle();
unsigned char *get_video_mem();

#endif
