#include "graphics.h"

#include <locale.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <wchar.h>

#define ALL_ON "█"
#define ALL_OFF " "
#define TOP "▀"
#define BOTTOM "▄"
#define SPACE(p)     \
    {                \
        p[0] = 0x20; \
        p[1] = 0;    \
        p[2] = 0;    \
        p[3] = 0;    \
    }
#define XSET_MESSAGE "Please run 'xset r rate 100' for better keyboard input"
#define XSET_MESSAGE_TIME 10

#define WIDHT 128
#define HEIGHT 64
#define DRAW_BORDER() draw_centered_border(HEIGHT / 2 + 1, WIDHT + 1)

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

void draw_pixel(int y, int x, bool is_on) {
    int h, w;
    getmaxyx(stdscr, h, w);
    int start_x = (w - WIDHT) / 2;
    int start_y = (h - HEIGHT / 2) / 2;
    unsigned char codepoint = mvinch(start_y + y / 2, start_x + x);
    char pixel[] = {0xe2, 0x96, codepoint & 0xFF, 0};
    if (codepoint == ' ') {
        if (!is_on) return;
        pixel[2] = 0x80 + (y % 2) * 4;
    }
    switch (codepoint & 0xFF) {
        case 0x80:
            if (y % 2 == 0 && is_on) return;
            if (y % 2 == 1 && !is_on) return;
            if (y % 2 == 0 && !is_on) {
                SPACE(pixel); 
                break;
            }
            pixel[2] = 0x88;
            break;
        case 0x84:
            if (y % 2 == 1 && is_on) return;
            if (y % 2 == 0 && !is_on) return;
            if (y % 2 == 1 && !is_on) {
                SPACE(pixel); 
                break;
            }
            pixel[2] = 0x88;
            break;
        case 0x88:
            if (is_on) return;
            pixel[2] = 0x80 + ((y + 1) % 2) * 4;
            break;
    }
    mvaddstr(start_y + y / 2, start_x + x, pixel);
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

// I want to kill myself
void draw(unsigned char *video_mem, unsigned int video_signal) {
    unsigned short num_byte = (video_signal & 0xFFFF00) >> 8;
    num_byte--;
    unsigned char n = (video_signal & 0x00F0) >> 4;
    int x = num_byte % 16;
    int y = num_byte / 16;
    if (n == 0) n = 16;
    for (int i = 0; i < n; i++) {
        if (y + i >= HEIGHT) break;
        unsigned int curr_int = video_mem[num_byte + i * 16] << 24;
        curr_int |= video_mem[num_byte + i * 16 + 1] << 16;
        curr_int |= video_mem[num_byte + i * 16 + 2] << 8;
        curr_int |= video_mem[num_byte + i * 16 + 3];
        for (int j = 0; j < 32; j++) {
            draw_pixel(y + i, x * 8 + j, (curr_int & 0x80000000) >> 31);
            curr_int <<= 1;
            if (x == 15 && j >= 7) break;
            if (x == 14 && j >= 15) break;
            if (x == 13 && j >= 23) break;
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
    int y = (h - HEIGHT / 2) / 4;
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

void pattern() {
    static bool should_switch;
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 128; j++) {
            draw_pixel(i, j, (i + j) % 2 == should_switch);
        }
    }
    should_switch = !should_switch;
    refresh();
}
