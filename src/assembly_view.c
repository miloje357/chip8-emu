#include "assembly_view.h"

#include <ctype.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "dasm.h"

#define AV_REFRESH()                                            \
    prefresh(assembly_view, first_row, 0, av_starty, av_startx, \
             av_starty + av_height - 1, av_startx + av_width - 1)
#define IS_INST_LABELED(stat) \
    (!((stat).is_directive) && strlen((stat).label) != 0)
#define ABS(x) ((x > 0) ? x : -x)
#define TO_FIRST_LABELED(up) \
    while (first_row >= 0 && \
           mvwinch(assembly_view, first_row += ((up) ? -1 : 1), 5) != ' ')

typedef enum {
    NOT_SELECTED,
    SELECTED,
    NAME,
    REGISTER,
    IMMEDIATE,
    ADDRESS,
} SyntaxColor;

AsmStatement *assembly;
size_t num_statements;
unsigned short last_pc;

WINDOW *assembly_view;
int av_startx, av_starty, av_width, av_height;
size_t num_rows;
int first_row = 0;

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

size_t get_num_rows() {
    size_t rows = 0;
    for (int i = 0; i < num_statements; i++) {
        rows++;
        if (IS_INST_LABELED(assembly[i])) rows += 2;
    }
    return rows;
}

SyntaxColor get_arg_color(char *arg) {
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
        SyntaxColor color = get_arg_color(stat.args[i]);
        if (!is_selected) wattron(assembly_view, COLOR_PAIR(color));
        wprintw(assembly_view, " %s,", stat.args[i]);
        if (!is_selected) wattroff(assembly_view, COLOR_PAIR(color));
    }

    // Draw last argument
    if (stat.num_args != 0) {
        SyntaxColor color = get_arg_color(stat.args[stat.num_args - 1]);
        if (!is_selected) wattron(assembly_view, COLOR_PAIR(color));
        wprintw(assembly_view, " %s", stat.args[stat.num_args - 1]);
        if (!is_selected) wattroff(assembly_view, COLOR_PAIR(color));
    }

    if (is_selected) wattroff(assembly_view, COLOR_PAIR(SELECTED));
    row++;
    wrefresh(assembly_view);
    return row;
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

void init_assembly_graphics() {
    start_color();
    init_pair(SELECTED, COLOR_BLACK, COLOR_WHITE);
    use_default_colors();
    init_pair(NOT_SELECTED, -1, -1);
    init_pair(NAME, COLOR_GREEN, -1);
    init_pair(REGISTER, COLOR_RED, -1);
    init_pair(IMMEDIATE, COLOR_BLUE, -1);
    init_pair(ADDRESS, COLOR_YELLOW, -1);
    num_rows = get_num_rows();
    assembly_view = newpad(num_rows + av_height, av_width);
    draw_assembly();
}

void delete_assembly_graphics() {
    if (assembly_view == NULL) return;
    delwin(assembly_view);
    assembly_view = NULL;
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
