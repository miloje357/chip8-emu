/* TODO: 1. Implement assembly buffer
 */
#include "debugger.h"

// #include <ncurses.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "chip8.h"

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_RESET "\x1b[0m"

const char *err_msg = NULL;
DebugType debug_state = NO_DEBUGGING;

void set_debugging(DebugType type) { debug_state = type; }

DebugType get_debugging() { return debug_state; }

void debug_printf(const char *format, ...) {
    // char *str;
    va_list args;

    switch (debug_state) {
        case NO_DEBUGGING:
            return;
        case GRAPHIC_DEBUGGING:
            // NOTE: Should delete before merge
            /*
            va_start(args, format);
            vasprintf(&str, format, args);
            va_end(args);
            mvprintw(0, 133, "%s", str);
            */
            return;
        case CONSOLE_DEBUGGING:
            va_start(args, format);
            vprintf(format, args);
            va_end(args);
    }
}

void print_registers(unsigned char *regs) {
    printf("Registers:       ");
    for (int i = 0; i < 16; i++) {
        printf("%02x   ", regs[i]);
    }
}

void print_stack(unsigned char *stack, unsigned char stack_size,
                 unsigned char sp) {
    printf("Stack:           ");
    for (int i = 0; i < stack_size; i += 2) {
        if (i == sp) {
            printf(ANSI_COLOR_RED "%04x " ANSI_COLOR_RESET,
                   *(unsigned short *)(stack + i));
            continue;
        }
        printf("%04x ", *(unsigned short *)(stack + i));
    }
}

void print_memory(unsigned char *memory, unsigned short pc) {
    printf("Memory:\n");
    for (int i = 0; i < 64; i++) {
        printf("%04x  ", PROGRAM_START + i * 16);
        for (int j = 0; j < 16; j++) {
            unsigned short index = PROGRAM_START + i * 16 + j;
            if (pc - 2 == index || pc - 1 == index) {
                printf(ANSI_COLOR_RED "%02x " ANSI_COLOR_RESET, memory[index]);
            } else
                printf("%02x ", memory[index]);
            if (j == 7) printf(" ");
        }
        printf("          %04x  ", START_VIDEO_MEM + i * 16);
        for (int j = 0; j < 16; j++) {
            unsigned short index = START_VIDEO_MEM + i * 16 + j;
            printf("%02x ", memory[index]);
            if (j == 7) printf(" ");
        }
        printf("\n");
    }
}

void print_state(Chip8Context *chip8) {
    print_registers(chip8->V);
    printf("\n");
    print_stack(chip8->memory + STACK_START, STACK_END - STACK_START,
                chip8->sp);
    printf("\n");
    printf("Stack pointer:   %02x\n", chip8->sp);
    printf("Program counter: %04x\n", chip8->pc);
    printf("Index register:  %04x\n", chip8->I);
    printf("\n");
    print_memory(chip8->memory, chip8->pc);
}

void set_error(const char *new_err_msg) { err_msg = new_err_msg; }

void print_error() {
    if (err_msg == NULL) return;
    printf("[CRASH] %s\n", err_msg);
}
