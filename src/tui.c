#include <locale.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "debugger.h"
#include "game_graphics.h"

#define SECONDS 1000000

#define DEBUG_RATIO 0.8
#define DEBUG_STARTY 0
#define SMALL_WINDOW_MESSAGE "Please resize the window"

bool has_debugging;
int width, height;

void clear_area(int y, int x, int h, int w) {
    char *clear_buffer = malloc(sizeof(char) * w + 1);
    memset(clear_buffer, ' ', w);
    clear_buffer[w] = '\0';
    for (int i = 0; i < h; i++) {
        mvaddstr(y + i, x, clear_buffer);
    }
    free(clear_buffer);
}

void set_win_dimens(int *game_width, int *game_height) {
    struct winsize ws;
    ioctl(0, TIOCGWINSZ, &ws);
    width = ws.ws_col;
    height = ws.ws_row;
    int gw = (has_debugging) ? width * DEBUG_RATIO : width;
    int gh = height;
    if (game_width != NULL) {
        *game_width = gw;
    }
    if (game_height != NULL) {
        *game_height = gw;
    }
    set_game_dimens(gh, gw);
    set_debug_dimes(DEBUG_STARTY, gw, height, width - gw);
}

void init_graphics(bool has_debugging_) {
    has_debugging = has_debugging_;
    setlocale(LC_CTYPE, "");
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, true);
    curs_set(0);
    set_win_dimens(NULL, NULL);
    init_game_graphics();
    if (has_debugging) init_debug_graphics();
    refresh();
}

void draw_border(int y, int x, int h, int w) {
    mvhline(y, x, 0, w);
    mvhline(y + h, x, 0, w);
    mvvline(y, x, 0, h);
    mvvline(y, x + w, 0, h);
    mvaddch(y, x, ACS_ULCORNER);
    mvaddch(y + h, x, ACS_LLCORNER);
    mvaddch(y, x + w, ACS_URCORNER);
    mvaddch(y + h, x + w, ACS_LRCORNER);
}

void draw_game_border(int h, int w) {
    draw_border((height - h) / 2, (width - w) / 2, h - 1, w - 1);
}

void display_small_window_message() {
    clear();
    draw_game_border(3, strlen(SMALL_WINDOW_MESSAGE) + 2);
    mvaddstr((height - 1) / 2, (width - strlen(SMALL_WINDOW_MESSAGE)) / 2,
             SMALL_WINDOW_MESSAGE);
    refresh();
}

void redraw_all(unsigned char *video_mem, bool hi_res) {
    redraw_game(video_mem, hi_res);
    if (has_debugging) redraw_debug();
}

void handle_win_size(unsigned char *video_mem, bool hi_res) {
    static int last_win_h, last_win_w;
    int game_width, game_height;
    set_win_dimens(&game_width, &game_height);
    if (last_win_w == width && last_win_h == height) return;
    while (game_height < GAME_HEIGHT || game_width < GAME_WIDTH) {
        set_win_dimens(&game_width, &game_height);
        if (last_win_w != width || last_win_h != height) {
            display_small_window_message();
            last_win_w = width;
            last_win_h = height;
        }
        usleep(0.5 * SECONDS);
    }
    redraw_all(video_mem, hi_res);
    last_win_w = width;
    last_win_h = height;
}
