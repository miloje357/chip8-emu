/* TODO: 1. Display message when a key must be pressed in debugging mode
 *       2. Add scrolling
 *       3. Add state view
 *       4. Add breakpoints
 *       5. Add a way to turn on and off assembly and state view
 */
#include "debugger.h"

#include <ctype.h>
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

#define REDRAW 0
typedef enum {
    NOT_SELECTED,
    SELECTED,
    NAME,
    REGISTER,
    IMMEDIATE,
    ADDRESS,
} Color;

#define DRAW_LINE() mvvline(d_starty, d_startx, 0, d_height)

const char *err_msg = NULL;
DebugType debug_state = NO_DEBUGGING;

AsmStatement *assembly;
size_t num_statements;
int d_startx, d_starty, d_width, d_height;
unsigned short last_pc;
unsigned int first_inst_index = 0;

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

Color get_arg_color(char *arg) {
    if (arg[0] == 'L') return ADDRESS;
    if (isalpha(arg[0])) return REGISTER;
    return IMMEDIATE;
}

int draw_statement(int row, AsmStatement stat, bool is_selected) {
    int x = d_startx + 1;
    move(row, x);

    if (stat.is_directive) {
        // Draw label
        if (!is_selected) attron(COLOR_PAIR(ADDRESS));
        printw("%s: ", stat.label);
        if (!is_selected) attroff(COLOR_PAIR(ADDRESS));

        // Draw bytes
        if (!is_selected) attron(COLOR_PAIR(IMMEDIATE));
        printw("%s %s", stat.name, stat.args[0]);
        for (int i = 1; i < stat.num_args; i++) {
            printw(" %s", stat.args[i]);
        }
        if (!is_selected) attroff(COLOR_PAIR(IMMEDIATE));

        row++;
        return row;
    }

    // Draw label
    if (strlen(stat.label) != 0) {
        attron(COLOR_PAIR(ADDRESS));
        mvprintw(row + 1, x, "%s:", stat.label);
        attroff(COLOR_PAIR(ADDRESS));

        row += 2;
        move(row, x);
    }

    if (is_selected) attron(COLOR_PAIR(SELECTED));
    // Draw name
    if (!is_selected) attron(COLOR_PAIR(NAME));
    printw("\t%s", stat.name);
    if (!is_selected) attroff(COLOR_PAIR(NAME));

    // Draw arguments
    for (int i = 0; i < stat.num_args - 1; i++) {
        Color color = get_arg_color(stat.args[i]);
        if (!is_selected) attron(COLOR_PAIR(color));
        printw(" %s,", stat.args[i]);
        if (!is_selected) attroff(COLOR_PAIR(color));
    }

    // Draw last argument
    if (stat.num_args != 0) {
        Color color = get_arg_color(stat.args[stat.num_args - 1]);
        if (!is_selected) attron(COLOR_PAIR(color));
        printw(" %s", stat.args[stat.num_args - 1]);
        if (!is_selected) attroff(COLOR_PAIR(color));
    }

    if (is_selected) attroff(COLOR_PAIR(SELECTED));
    row++;
    refresh();
    return row;
}

int index_to_addr(int index) {
    int addr = 0x200;
    for (int i = 0; i < index; i++) {
        if (assembly[i].is_directive)
            addr += assembly[i].num_args;
        else
            addr += 2;
    }
    return addr;
}

int addr_to_index(int target) {
    int addr = 0x200;
    for (int i = 0; i < num_statements; i++) {
        if (addr == target) return i;
        if (assembly[i].is_directive)
            addr += assembly[i].num_args;
        else
            addr += 2;
    }
    return -1;
}

// TODO: Optimize with a lookup table
void draw_assembly() {
    int row = d_starty;
    // addr must start at the address of the first displayed statement
    int addr = index_to_addr(first_inst_index);

    clear_area(d_starty, d_startx + 1, d_height, d_width);
    for (int i = first_inst_index;
         i < num_statements && row < d_height - d_starty; i++) {
        row = draw_statement(row, assembly[i], addr == last_pc);
        if (assembly[i].is_directive)
            addr += assembly[i].num_args;
        else
            addr += 2;
    }
}

void set_curr_inst(unsigned short pc) {
    int row = d_starty;
    int addr = 0x200;
    for (int i = 0; i < num_statements; i++) {
        // row to be selected is out of bounds
        if ((row >= d_height - d_starty || i < first_inst_index) &&
            addr == pc) {
            row = 0;
            first_inst_index = addr_to_index(addr);
            // skip to first label
            while (strlen(assembly[first_inst_index--].label) == 0) {
                row++;
            }
            // Scroll by one to hide an unwanted instruction
            first_inst_index++;
            // if the label is too far, scroll to the current instruction
            if (row >= d_height - d_starty - 5) {
                first_inst_index = addr_to_index(addr) - 3;
            }
            last_pc = pc;
            draw_assembly();
            return;
        }
        if (addr == pc || addr == last_pc) {
            // deselects instruction when addr == last_pc
            draw_statement(row, assembly[i], addr == pc);
        }
        if (assembly[i].is_directive) {
            addr += assembly[i].num_args;
        } else {
            addr += 2;
        }
        // don't update the row until instructions are visible
        if (i < first_inst_index) continue;
        row++;
        if (!assembly[i].is_directive && strlen(assembly[i].label) != 0)
            row += 2;
    }
    last_pc = pc;
}

void init_debug_graphics() {
    start_color();
    init_pair(SELECTED, COLOR_BLACK, COLOR_WHITE);
    use_default_colors();
    init_pair(NOT_SELECTED, -1, -1);
    init_pair(NAME, COLOR_GREEN, -1);
    init_pair(REGISTER, COLOR_RED, -1);
    init_pair(IMMEDIATE, COLOR_BLUE, -1);
    init_pair(ADDRESS, COLOR_YELLOW, -1);
    draw_assembly();
}

void redraw_debug() {
    draw_assembly();
    DRAW_LINE();
}
