#include "graphics.h"

#include <locale.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define PIXEL_ON "██"
#define PIXEL_OFF "  "
#define XSET_MESSAGE "Please run 'xset r rate 100' for better keyboard input"
#define XSET_MESSAGE_TIME 10

#define WIDHT 64
#define HEIGHT 32
#define DRAW_BORDER() draw_centered_border(HEIGHT + 1, WIDHT * 2 + 1)

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

void draw_centered_border(int h, int w) {
    int scr_h, scr_w;
    getmaxyx(stdscr, scr_h, scr_w);
    draw_border((scr_h - h) / 2, (scr_w - w) / 2, h, w);
}

void draw_pixel(int y, int x, const char *pixel) {
    int h, w;
    getmaxyx(stdscr, h, w);
    mvaddstr((h - HEIGHT) / 2 + y, w / 2 + x * 2 - WIDHT, pixel);
}

void init_graphics() {
    setlocale(LC_CTYPE, "");
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, true);
    curs_set(0);
    DRAW_BORDER();
    refresh();
}

void draw(unsigned char *video_mem, unsigned short video_signal) {
    unsigned char num_byte = (video_signal & 0xFF00) >> 8;
    unsigned char n = (video_signal & 0x00F0) >> 4;
    int x = num_byte % 8;
    int y = num_byte / 8;
    for (int i = 0; i < n; i++) {
        if (y + i >= HEIGHT) break;
        unsigned short curr_short = video_mem[num_byte + i * 8] << 8;
        curr_short += video_mem[num_byte + i * 8 + 1];
        for (int j = 0; j < 16; j++) {
            const char *pixel = (curr_short & 0x8000) ? PIXEL_ON : PIXEL_OFF;
            draw_pixel(y + i, x * 8 + j, pixel);
            curr_short <<= 1;
            if (x == 7 && j >= 7) break;
        }
    }
    refresh();
}

void clear_screen() {
    clear();
    DRAW_BORDER();
    refresh();
}

void st_flash(bool is_pixel_on) {
    static bool was_pixel_on = false;
    if (was_pixel_on == is_pixel_on) {
        return;
    }
    was_pixel_on = is_pixel_on;

    int h, w;
    getmaxyx(stdscr, h, w);
    char pixel = (is_pixel_on) ? '@' : ' ';
    char *pixels = malloc(w * (h - HEIGHT - 2) / 2 + 1);
    memset(pixels, pixel, w * (h - HEIGHT - 2) / 2);
    pixels[w * (h - HEIGHT - 2) / 2 + 1] = '\0';
    mvaddstr(0, 0, pixels);
    mvaddstr((h + HEIGHT) / 2 + 1, 0, pixels);
    free(pixels);
    pixels = malloc(w / 2 - WIDHT);
    memset(pixels, pixel, w / 2 - WIDHT - 1);
    pixels[w / 2 - WIDHT - 1] = '\0';
    for (int i = -1; i <= HEIGHT; i++) {
        mvaddstr((h - HEIGHT) / 2 + i, 0, pixels);
        mvaddstr((h - HEIGHT) / 2 + i, w / 2 + WIDHT + 1, pixels);
    }
    refresh();
    free(pixels);
}

unsigned long get_secs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

void display_xset_message() {
    int h, w;
    getmaxyx(stdscr, h, w);
    int y = (h - HEIGHT) / 4;
    int x = (w - strlen(XSET_MESSAGE)) / 2;
    draw_border(y - 1, x - 1, 2, strlen(XSET_MESSAGE) + 1);
    mvaddstr(y, x, XSET_MESSAGE);
    refresh();
}

void clear_xset_message() {
    int h, w;
    getmaxyx(stdscr, h, w);
    int y = (h - HEIGHT) / 4 - 1;
    int x = (w - strlen(XSET_MESSAGE)) / 2 - 1;
    for (int i = 0; i < 3; i++) {
        move(y + i, x);
        clrtoeol();
    }
}

void handle_xset_message() {
    static unsigned long first_called;
    static bool should_display = true;
    if (!should_display) return;
    if (first_called == 0) first_called = get_secs();
    if (get_secs() - first_called > XSET_MESSAGE_TIME) {
        should_display = false;
        clear_xset_message();
        return;
    }
    display_xset_message();
}
