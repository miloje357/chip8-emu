/* TODO: 1. Add 'key needs to be pressed' window in state view
 *       2. Add breakpoints and a 'add breakpoint' window to state view
 *       3. Display message when a key must be pressed in debugging mode
 */
#include "debugger.h"

#include <ctype.h>
#include <ncurses.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "chip8.h"
#include "dasm.h"

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_RESET "\x1b[0m"

typedef enum {
    NOT_SELECTED,
    SELECTED,
    NAME,
    REGISTER,
    IMMEDIATE,
    ADDRESS,
    STACK_POINTER,
} Color;

#define AV_REFRESH()                                            \
    prefresh(assembly_view, first_row, 0, av_starty, av_startx, \
             av_starty + av_height - 1, av_startx + av_width - 1)
#define IS_INST_LABELED(stat) \
    (!((stat).is_directive) && strlen((stat).label) != 0)
#define ABS(x) ((x > 0) ? x : -x)
#define TO_FIRST_LABELED(up) \
    while (first_row >= 0 && \
           mvwinch(assembly_view, first_row += ((up) ? -1 : 1), 5) != ' ')

const char *err_msg = NULL;
DebugType debug_state = NO_DEBUGGING;

AsmStatement *assembly;
size_t num_statements;
unsigned short last_pc;

WINDOW *assembly_view;
int av_startx, av_starty, av_width, av_height;
size_t num_rows;
int first_row = 0;

WINDOW *state_view;
int sv_startx, sv_starty, sv_width, sv_height;

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

void set_assembly_dimens(int y, int x, int h, int w) {
    av_starty = y;
    av_startx = x;
    av_height = h;
    av_width = w;
}

void set_state_dimens(int y, int x, int h, int w) {
    sv_starty = y;
    sv_startx = x;
    sv_height = h;
    sv_width = w;
}

Color get_arg_color(char *arg) {
    if (arg[0] == 'L') return ADDRESS;
    if (isalpha(arg[0])) return REGISTER;
    return IMMEDIATE;
}

int draw_statement(int row, AsmStatement stat, bool is_selected) {
    int x = 1;
    wmove(assembly_view, row, x);

    if (stat.is_directive) {
        wprintw(assembly_view, "%3d ", row + 1);
        // Draw label
        if (!is_selected) wattron(assembly_view, COLOR_PAIR(ADDRESS));
        wprintw(assembly_view, "%s: ", stat.label);
        if (!is_selected) wattroff(assembly_view, COLOR_PAIR(ADDRESS));

        // Draw bytes
        if (!is_selected) wattron(assembly_view, COLOR_PAIR(IMMEDIATE));
        wprintw(assembly_view, "%s %s", stat.name, stat.args[0]);
        for (int i = 1; i < stat.num_args; i++) {
            wprintw(assembly_view, " %s", stat.args[i]);
        }
        if (!is_selected) wattroff(assembly_view, COLOR_PAIR(IMMEDIATE));

        row++;
        return row;
    }

    // Draw label
    if (strlen(stat.label) != 0) {
        wprintw(assembly_view, "%3d ", row + 1);
        mvwprintw(assembly_view, row + 1, x, "%3d ", row + 2);
        wattron(assembly_view, COLOR_PAIR(ADDRESS));
        wprintw(assembly_view, "%s:", stat.label);
        wattroff(assembly_view, COLOR_PAIR(ADDRESS));

        row += 2;
        wmove(assembly_view, row, x);
    }

    wprintw(assembly_view, "%3d ", row + 1);
    if (is_selected) wattron(assembly_view, COLOR_PAIR(SELECTED));
    // Draw name
    if (!is_selected) wattron(assembly_view, COLOR_PAIR(NAME));
    wprintw(assembly_view, "\t%s", stat.name);
    if (!is_selected) wattroff(assembly_view, COLOR_PAIR(NAME));

    // Draw arguments
    for (int i = 0; i < stat.num_args - 1; i++) {
        Color color = get_arg_color(stat.args[i]);
        if (!is_selected) wattron(assembly_view, COLOR_PAIR(color));
        wprintw(assembly_view, " %s,", stat.args[i]);
        if (!is_selected) wattroff(assembly_view, COLOR_PAIR(color));
    }

    // Draw last argument
    if (stat.num_args != 0) {
        Color color = get_arg_color(stat.args[stat.num_args - 1]);
        if (!is_selected) wattron(assembly_view, COLOR_PAIR(color));
        wprintw(assembly_view, " %s", stat.args[stat.num_args - 1]);
        if (!is_selected) wattroff(assembly_view, COLOR_PAIR(color));
    }

    if (is_selected) wattroff(assembly_view, COLOR_PAIR(SELECTED));
    row++;
    wrefresh(assembly_view);
    return row;
}

size_t get_num_rows() {
    size_t rows = 0;
    for (int i = 0; i < num_statements; i++) {
        rows++;
        if (IS_INST_LABELED(assembly[i])) rows += 2;
    }
    return rows;
}

void draw_assembly() {
    int row = 0;
    unsigned short addr = 0x200;

    wclear(assembly_view);
    for (int i = 0; i < num_statements; i++) {
        row = draw_statement(row, assembly[i], addr == last_pc);
        if (assembly[i].is_directive)
            addr += assembly[i].num_args;
        else
            addr += 2;
    }

    mvwvline(assembly_view, 0, 0, 0, num_rows + av_height);
    AV_REFRESH();
}

// TODO: Optimize
int addr_to_row(unsigned short target) {
    int row = 0;
    unsigned short addr = 0x200;

    for (int i = 0; i < num_statements; i++) {
        if (addr == target) return row;

        if (assembly[i].is_directive)
            addr += assembly[i].num_args;
        else
            addr += 2;

        row++;
        if (IS_INST_LABELED(assembly[i])) row += 2;
    }
    return -1;
}

// TODO: Optimize
AsmStatement *addr_to_stat(unsigned short target) {
    unsigned short addr = 0x200;

    for (int i = 0; i < num_statements; i++) {
        if (addr == target) return &assembly[i];
        if (assembly[i].is_directive)
            addr += assembly[i].num_args;
        else
            addr += 2;
    }
    return NULL;
}

void set_curr_inst(unsigned short pc) {
    int last_selected_row = addr_to_row(last_pc);
    int curr_selected_row = addr_to_row(pc);
    AsmStatement *last_selected_stat = addr_to_stat(last_pc);
    AsmStatement *curr_selected_stat = addr_to_stat(pc);

    draw_statement(curr_selected_row, *curr_selected_stat, true);
    if (last_selected_stat != NULL)
        draw_statement(last_selected_row, *last_selected_stat, false);

    // Is selected row is out of bounds?
    if (curr_selected_row < first_row ||
        curr_selected_row > first_row + av_height * 0.75) {
        first_row = curr_selected_row;
        TO_FIRST_LABELED(true);
        if (curr_selected_row - first_row >= av_height)
            first_row = curr_selected_row;
    }
    AV_REFRESH();
    last_pc = pc;
}

void scroll_by(ScrollUnit unit, int num) {
    switch (unit) {
        case LINE:
            first_row += num;
            break;
        case LABEL:
            for (int i = 0; i < ABS(num); i++) TO_FIRST_LABELED(num < 0);
            break;
        case TOP:
            first_row = 0;
            break;
        case BOTTOM:
            first_row = num_rows - av_height;
            break;
    }
    if (first_row < 0) first_row = 0;
    if (first_row > num_rows - av_height) first_row = num_rows - av_height;

    AV_REFRESH();
}

void draw_registers(unsigned char *regs, bool has_been_init,
                    bool should_draw_last) {
    static unsigned char last_regs[16];
    const char *registers_msg = "Registers:       ";
    mvwaddstr(state_view, 1, 0, registers_msg);
    if (should_draw_last) {
        for (int i = 0; i < 16; i++) {
            mvwprintw(state_view, 1, strlen(registers_msg) + i * 5, "%02x   ",
                      last_regs[i]);
        }
        return;
    }
    for (int i = 0; i < 16; i++) {
        if (has_been_init && regs[i] == last_regs[i]) continue;
        mvwprintw(state_view, 1, strlen(registers_msg) + i * 5, "%02x   ",
                  regs[i]);
    }
    memcpy(last_regs, regs, 16);
}

// NOTE: Stack size is always 16
void draw_stack(unsigned char *stack, unsigned char sp, bool has_been_init,
                bool should_draw_last) {
    static unsigned char last_sp;
    static unsigned char last_stack[32];
    const char *stack_msg = "Stack:           ";
    mvwaddstr(state_view, 2, 0, stack_msg);
    if (should_draw_last) {
        for (int i = 0; i < 32; i += 2) {
            if (i == last_sp) wattron(state_view, COLOR_PAIR(STACK_POINTER));
            mvwprintw(state_view, 2, strlen(stack_msg) + i / 2 * 5, "%04x ",
                      ((unsigned short *)last_stack)[i]);
            if (i == last_sp) wattroff(state_view, COLOR_PAIR(STACK_POINTER));
        }
        return;
    }
    for (int i = 0; i < 32; i += 2) {
        if (has_been_init &&
            ((unsigned short *)stack)[i] == ((unsigned short *)last_stack)[i] &&
            last_sp == sp && sp != i)
            continue;
        if (i == sp) wattron(state_view, COLOR_PAIR(STACK_POINTER));
        mvwprintw(state_view, 2, strlen(stack_msg) + i / 2 * 5, "%04x ",
                  ((unsigned short *)stack)[i]);
        if (i == sp) wattroff(state_view, COLOR_PAIR(STACK_POINTER));
    }
    memcpy(last_stack, stack, 16 * 2);
    last_sp = sp;
}

void draw_special(unsigned char sp, unsigned short pc, unsigned short I,
                  unsigned char dt, unsigned char st, bool has_been_init,
                  bool should_draw_last) {
    static unsigned char last_sp;
    static unsigned short last_pc;
    static unsigned short last_I;
    static unsigned char last_dt, last_st;
    if (should_draw_last) {
        mvwprintw(state_view, 3, 0, "Stack pointer:   %02x", last_sp);
        mvwprintw(state_view, 4, 0, "Program counter: %04x", last_pc);
        mvwprintw(state_view, 5, 0, "Index register:  %04x", last_I);
        mvwprintw(state_view, 6, 0, "Delay timer:     %02x", last_dt);
        mvwprintw(state_view, 7, 0, "Sound timer:     %02x", last_st);
        return;
    }
    if (!has_been_init || last_sp != sp)
        mvwprintw(state_view, 3, 0, "Stack pointer:   %02x", sp);
    if (!has_been_init || last_pc != pc)
        mvwprintw(state_view, 4, 0, "Program counter: %04x", pc);
    if (!has_been_init || last_I != I)
        mvwprintw(state_view, 5, 0, "Index register:  %04x", I);
    if (!has_been_init || last_dt != dt)
        mvwprintw(state_view, 6, 0, "Delay timer:     %02x", dt);
    if (!has_been_init || last_st != st)
        mvwprintw(state_view, 7, 0, "Sound timer:     %02x", st);
    last_sp = sp;
    last_pc = pc;
    last_I = I;
    last_dt = dt;
    last_st = st;
}

void draw_state(Chip8Context *chip8) {
    static bool has_been_init;

    if (chip8 == NULL) {
        draw_registers(NULL, has_been_init, true);
        draw_special(0, 0, 0, 0, 0, has_been_init, true);
        draw_stack(NULL, 0, has_been_init, true);
        mvwhline(state_view, 0, 0, 0, sv_width);
        wrefresh(state_view);
        return;
    }

    draw_registers(chip8->V, has_been_init, false);
    draw_special(chip8->sp, chip8->pc, chip8->I, chip8->dt, chip8->st,
                 has_been_init, false);
    draw_stack(chip8->memory + STACK_START, chip8->sp, has_been_init, false);

    wrefresh(state_view);
    has_been_init = true;
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
    init_pair(STACK_POINTER, COLOR_RED, -1);
    num_rows = get_num_rows();
    assembly_view = newpad(num_rows + av_height, av_width);
    state_view = newwin(sv_height, sv_width, sv_starty, sv_startx);
    draw_assembly();
    draw_state(NULL);
}

void delete_debug_graphics() {
    if (assembly_view == NULL) return;
    delwin(assembly_view);
    assembly_view = NULL;

    if (state_view == NULL) return;
    delwin(state_view);
    state_view = NULL;
}
