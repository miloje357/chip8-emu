#ifndef CHIP8_H_
#define CHIP8_H_

int load_program(const char* program_path);
void print_state();
void init_chip8();
int next_cycle();

#endif
