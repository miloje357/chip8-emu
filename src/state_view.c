#include "state_view.h"

#include <assert.h>
#include <ncurses.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "chip8.h"

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define STACK_POINTER 10

#define MAX_STATE_LABEL_LEN 17
#define MAX_STATE_ENTRY_LEN (MAX_STATE_LABEL_LEN + 5 * 16)

#define SET_MV_BG()                                  \
    for (int i = 0; i < mv_height * mv_width; i++) { \
        waddstr(message_view, "╱");                  \
    }

const char *err_msg = NULL;
DebugType debug_state = NO_DEBUGGING;

WINDOW *state_view;
int sv_startx, sv_starty, sv_width, sv_height;

WINDOW *message_view;
int mv_startx, mv_starty, mv_width, mv_height;

void set_debugging(DebugType type) { debug_state = type; }

DebugType get_debugging() { return debug_state; }

void debug_printf(const char *format, ...) {
    if (debug_state != CONSOLE_DEBUGGING) return;
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
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

void set_state_dimens(int y, int x, int h, int w) {
    sv_starty = y;
    sv_startx = x;
    sv_height = h;
    sv_width = w;
}

void draw_registers(unsigned char *regs, bool has_been_init,
                    bool should_draw_last) {
    static unsigned char last_regs[16];
    const char *registers_msg = "Registers:       ";
    mvwaddstr(state_view, 1, 0, registers_msg);
    if (should_draw_last) {
        for (int i = 0; i < 16; i++) {
            mvwprintw(state_view, 1, MAX_STATE_LABEL_LEN + i * 5, "%02x   ",
                      last_regs[i]);
        }
        return;
    }
    for (int i = 0; i < 16; i++) {
        if (has_been_init && regs[i] == last_regs[i]) continue;
        mvwprintw(state_view, 1, MAX_STATE_LABEL_LEN + i * 5, "%02x   ",
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
            mvwprintw(state_view, 2, MAX_STATE_LABEL_LEN + i / 2 * 5, "%04x ",
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
        mvwprintw(state_view, 2, MAX_STATE_LABEL_LEN + i / 2 * 5, "%04x ",
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

// TODO: Add wraping text
// TODO: Add optional pause
void set_message(const char *message) {
    wclear(message_view);
    SET_MV_BG();
    WINDOW *message_win =
        derwin(message_view, 3, 2 + strlen(message), (mv_height - 3) / 2,
               (mv_width - strlen(message) - 2) / 2);
    wborder(message_win, 0, 0, 0, 0, 0, 0, 0, 0);
    mvwaddstr(message_win, 1, 1, message);
    delwin(message_win);
    wrefresh(message_view);
}

void clear_message_view() {
    wclear(message_view);
    wrefresh(message_view);
}

// NOTE: Free after use
// TODO: Add more movement options
char *get_from_field(const char *title, size_t *len) {
    // -2 for space around message_win, -2 for border
    size_t maxlen = mv_width - 2 - 2;
    char *buffer = malloc(maxlen);
    memset(buffer, 0, maxlen);

    wclear(message_view);
    SET_MV_BG();
    WINDOW *message_win =
        derwin(message_view, 3, mv_width - 2, (mv_height - 3) / 2, 1);
    nodelay(message_win, false);
    keypad(message_win, false);
    wclear(message_win);
    wborder(message_win, 0, 0, 0, 0, 0, 0, 0, 0);
    mvwaddstr(message_win, 0, 1, title);
    wrefresh(message_view);

    int key;
    *len = 0;
    wmove(message_win, 1, 1);
    curs_set(1);
    while ((key = wgetch(message_win)) != '\n') {
        if (key == '\x1b') {
            free(buffer);
            buffer = NULL;
            break;
        }
        curs_set(1);
        if (key == '\b' || key == KEY_BACKSPACE || key == 127) {
            if (*len <= 0) continue;
            mvwaddch(message_win, 1, *len, ' ');
            wmove(message_win, 1, *len);
            buffer[(*len)--] = '\0';
            continue;
        }
        if (*len >= maxlen || key >= 256 || key == '\t') {
            curs_set(0);
            continue;
        }
        buffer[(*len)++] = key;
        buffer[*len] = '\0';
        waddch(message_win, key);
        wrefresh(message_win);
    }

    curs_set(0);
    delwin(message_win);
    wclear(message_view);
    wrefresh(message_view);
    return buffer;
}

void init_state_graphics() {
    start_color();
    use_default_colors();
    init_pair(STACK_POINTER, COLOR_RED, -1);

    state_view = newwin(sv_height, sv_width, sv_starty, sv_startx);
    draw_state(NULL);

    mv_height = sv_height - 1;
    mv_width = sv_width - MAX_STATE_ENTRY_LEN - 1;
    mv_starty = 1;
    mv_startx = MAX_STATE_ENTRY_LEN + 1;
    message_view =
        derwin(state_view, mv_height, mv_width, mv_starty, mv_startx);
    mvwvline(state_view, mv_starty, mv_startx - 1, 0, mv_height);
    wrefresh(state_view);
}

void delete_state_graphics() {
    if (state_view == NULL) return;
    delwin(state_view);
    state_view = NULL;
}
