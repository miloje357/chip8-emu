#include "assembly_view.h"

#include <ctype.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "dasm.h"
#include "state_view.h"
#include "tui.h"

#define MAX_NUM_BREAKPOINTS 20
#define INST_STARTX 6

#define AV_REFRESH()                                            \
    prefresh(assembly_view, first_row, 0, av_starty, av_startx, \
             av_starty + av_height - 1, av_startx + av_width - 1)
#define IS_INST_LABELED(stat) \
    (!((stat).is_directive) && strlen((stat).label) != 0)
#define ABS(x) ((x > 0) ? x : -x)
#define TO_FIRST_LABELED(up) \
    while (first_row >= 0 && \
           mvwinch(assembly_view, first_row += ((up) ? -1 : 1), 5) != ' ')
#define DRAW_BREAKPOINT(row, exists)                     \
    if (exists) {                                        \
        wattron(assembly_view, COLOR_PAIR(BREAKPOINT));  \
        mvwaddstr(assembly_view, row, 1, "  ⬤  ");       \
        wattroff(assembly_view, COLOR_PAIR(BREAKPOINT)); \
    } else                                               \
        mvwprintw(assembly_view, row, 1, "%4d ", row + 1);
// NOTE: Works both with directives and instructions
#define IS_ROW_LABELED(row) \
    ((char)mvwinch(assembly_view, row, INST_STARTX) == 'L')

typedef enum {
    NOT_SELECTED,
    SELECTED,
    NAME,
    REGISTER,
    IMMEDIATE,
    ADDRESS,
    BREAKPOINT,
} SyntaxColor;

AsmStatement *assembly;
size_t num_statements;
unsigned short last_pc;

WINDOW *assembly_view;
int av_startx, av_starty, av_width, av_height;
size_t num_rows;
int first_row = 0;

size_t num_breakpoints;
int breakpoints[MAX_NUM_BREAKPOINTS] = {-1};

void init_assembly(FILE *src, bool has_quirks) {
    assembly = disassemble(src, &num_statements, has_quirks);
    memset(breakpoints, -1, MAX_NUM_BREAKPOINTS * sizeof(int));
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
    wmove(assembly_view, row, INST_STARTX);

    if (stat.is_directive) {
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
        wmove(assembly_view, row + 1, INST_STARTX);
        wattron(assembly_view, COLOR_PAIR(ADDRESS));
        wprintw(assembly_view, "%s:", stat.label);
        wattroff(assembly_view, COLOR_PAIR(ADDRESS));

        row += 2;
        wmove(assembly_view, row, INST_STARTX);
    }

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

void draw_line_numbers() {
    int curr_bp_index = num_breakpoints - 1;
    for (int row = num_rows - 1; row >= 0; row--) {
        if (curr_bp_index >= 0 && breakpoints[curr_bp_index] == row - 2 &&
            IS_ROW_LABELED(row - 1)) {
            DRAW_BREAKPOINT(row, true);
            curr_bp_index--;
            continue;
        }
        if (curr_bp_index >= 0 && breakpoints[curr_bp_index] == row) {
            DRAW_BREAKPOINT(row, true);
            curr_bp_index--;
            continue;
        }
        mvwprintw(assembly_view, row, 1, "%4d ", row + 1);
    }
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

    draw_line_numbers();
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
    init_pair(BREAKPOINT, COLOR_RED, -1);
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

// TODO: Add label breakpoints
int parse_bp_input(char *inp, size_t len, int *label_offset) {
    if (inp == NULL) return -1;
    char *end;
    int row = strtol(inp, &end, 0);
    if (end == inp || *end != '\0') {
        set_message("Invalid input: Not a number");
        return -1;
    }
    row--;
    if (row < 0 || row >= num_rows) {
        set_message("Invalid input: Out of bounds");
        return -1;
    }

    *label_offset = 0;
    if (IS_ROW_LABELED(row - 1)) {
        *label_offset = 2;
    }
    if (IS_ROW_LABELED(row)) {
        *label_offset = 1;
    }
    return row - *label_offset;
}

// returns false if the breakpoint already exists
bool insert_or_delete_breakpoint(int row) {
    int curr_row_index;
    for (curr_row_index = 0;
         breakpoints[curr_row_index] != -1 && breakpoints[curr_row_index] < row;
         curr_row_index++);
    if (breakpoints[curr_row_index] == row) {
        num_breakpoints--;
        memmove(&breakpoints[curr_row_index], &breakpoints[curr_row_index + 1],
                (num_breakpoints - curr_row_index) * sizeof(int));
        return false;
    }
    num_breakpoints++;
    memmove(&breakpoints[curr_row_index + 1], &breakpoints[curr_row_index],
            (num_breakpoints - curr_row_index) * sizeof(int));
    breakpoints[curr_row_index] = row;
    return true;
}

// TODO: Change to toggle_breakpoints
void toggle_breakpoint() {
    if (num_breakpoints >= MAX_NUM_BREAKPOINTS) {
        set_message("Reached maximum amount of breakpoints");
        return;
    }
    size_t bp_strlen;
    int offset;
    char *bp_input = get_from_field("Add breakpoint", &bp_strlen);
    int row = parse_bp_input(bp_input, bp_strlen, &offset);
    if (row == -1) return;
    free(bp_input);
    bool bp_inserted = insert_or_delete_breakpoint(row);
    DRAW_BREAKPOINT(row + offset, bp_inserted);
    AV_REFRESH();
}

void check_breakpoints(unsigned short pc) {
    int curr_row = addr_to_row(pc);
    int left = 0, right = num_breakpoints - 1;
    while (left <= right) {
        int mid = (left + right) / 2;
        if (breakpoints[mid] == curr_row) {
            // TODO: 'Reached a breakpoint' message in state_view
            set_debugging(GRAPHIC_DEBUGGING);
            reset_graphics(get_video_mem(), get_hi_res(), true);
            return;
        } else if (breakpoints[mid] < curr_row) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
}
