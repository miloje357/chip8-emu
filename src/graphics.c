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
#define DRAW_BORDER() draw_centered_border(HEIGHT / 2 + 2, WIDHT + 2)

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
    draw_border((scr_h - h) / 2, (scr_w - w) / 2, h - 1, w - 1);
}

void draw_pixel(int y, int x, bool is_on) {
    int h, w;
    getmaxyx(stdscr, h, w);
    const char *pixel = (is_on) ? PIXEL_ON : PIXEL_OFF;
    mvaddstr((h - HEIGHT / 2) / 2 + y, w / 2 + x * 2 - WIDHT / 2, pixel);
}

void draw_pixel_hi_res(int y, int x, bool is_on) {
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

void draw(unsigned char *video_mem, unsigned int video_signal, bool hi_res) {
    unsigned short num_byte = (video_signal & 0xFFFF00) >> 8;
    unsigned char n = (video_signal & 0x00F0) >> 4;
    int x = num_byte % 16;
    int y = num_byte / 16;
    if (n == 0) n = 16;
    int width = WIDHT;
    int height = HEIGHT;
    if (!hi_res) {
        width /= 2;
        height /= 2;
    }
    for (int i = 0; i < n && y + i < height; i++) {
        unsigned int curr_int = video_mem[x + (y + i) * 16] << 24;
        curr_int |= video_mem[x + (y + i) * 16 + 1] << 16;
        if (n == 16) curr_int |= video_mem[x + (y + i) * 16 + 2] << 8;
        for (int j = 0; j < 16 + ((n == 16) ? 8 : 0) && x * 8 + j < width;
             j++) {
            if (hi_res)
                draw_pixel_hi_res(y + i, x * 8 + j, (curr_int >> 31));
            else if ((x * 8) % 64 + j < width / 2)
                draw_pixel(y + i, (x * 8) % 64 + j, (curr_int >> 31));
            curr_int <<= 1;
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

void draw_all(unsigned char *video_mem, bool hi_res) {
    clear_screen();
    for (int num_byte = 0; num_byte < 0x400; num_byte++) {
        unsigned char curr_byte = *(video_mem + num_byte);
        int x = num_byte % 16;
        int y = num_byte / 16;
        if (!hi_res && x >= WIDHT / 8 / 2) continue;
        if (!hi_res && y >= HEIGHT / 2) break;
        for (int i = 0; i < 8; i++) {
            if (hi_res) {
                draw_pixel_hi_res(y, x * 8 + i, (curr_byte >> 7));
                curr_byte <<= 1;
                continue;
            }
            draw_pixel(y, (x * 8) % 64 + i, (curr_byte >> 7));
            curr_byte <<= 1;
        }
    }
}
