/* TODO: 1. Implement an assembly view
 *       2. Display message when a key must be pressed in debugging mode
 */
#include "debugger.h"

#include <ncurses.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chip8.h"
#include "dasm.h"
#include "tui.h"

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define DRAW_LINE() mvvline(d_starty, d_startx, 0, d_height)

const char *err_msg = NULL;
DebugType debug_state = NO_DEBUGGING;

AsmStatement *assembly;
size_t num_statements;
int d_startx, d_starty, d_width, d_height;

void set_debugging(DebugType type) { debug_state = type; }

DebugType get_debugging() { return debug_state; }

void debug_printf(const char *format, ...) {
    va_list args;

    switch (debug_state) {
        case NO_DEBUGGING:
        case GRAPHIC_DEBUGGING:
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

// TODO: Rewrite with GRAPHIC_DEBUGGING
void print_error() {
    if (err_msg == NULL) return;
    printf("[CRASH] %s\n", err_msg);
}

void set_assembly(FILE *src, bool has_quirks) {
    assembly = disassemble(src, &num_statements, has_quirks);
}

void free_assembly() {
    if (assembly == NULL) return;
    free(assembly);
}

void set_debug_dimes(int y, int x, int h, int w) {
    d_starty = y;
    d_startx = x;
    d_height = h;
    d_width = w;
}

int draw_statement(int row, AsmStatement stat) {
    int x = d_startx + 1;
    move(row, x);
    if (stat.is_directive) {
        printw("%s: %s %s", stat.label, stat.name, stat.args[0]);
        for (int i = 1; i < stat.num_args; i++) {
            printw(" %s", stat.args[i]);
        }
        row++;
        return row;
    }
    if (strlen(stat.label) != 0) {
        mvprintw(row + 1, x, "%s:", stat.label);
        row += 2;
        move(row, x);
    }
    printw("\t%s", stat.name);
    for (int i = 0; i < stat.num_args - 1; i++) {
        printw(" %s,", stat.args[i]);
    }
    if (stat.num_args != 0) {
        printw(" %s", stat.args[stat.num_args - 1]);
    }
    row++;
    return row;
    refresh();
}

void draw_assembly() {
    int row = d_starty;
    for (int i = 0; i < num_statements && row < d_height - d_starty; i++) {
        row = draw_statement(row, assembly[i]);
    }
}

void init_debug_graphics() {
    DRAW_LINE();
    draw_assembly();
}

void redraw_debug() {
    clear_area(d_starty, d_startx, d_height, d_width);
    DRAW_LINE();
    draw_assembly();
}
