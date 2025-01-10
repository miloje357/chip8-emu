#include "graphics.h"

#include <locale.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>

#include "chip8.h"

#define SECONDS 1000000

#define XSET_MESSAGE "Please run 'xset r rate 100' for better keyboard input"
#define XSET_MESSAGE_TIME 10
#define SMALL_WINDOW_MESSAGE "Please resize the window"

#define PIXEL_ON "██"
#define PIXEL_OFF "  "

#define REAL_WIDTH 128
#define REAL_HEIGHT 32
#define NUM_BYTES_IN_ROW (SIZE_VIDEO_MEM / HEIGTH)
#define DRAW_BORDER() draw_centered_border(REAL_HEIGHT + 2, REAL_WIDTH + 2)

int win_h, win_w;

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
    draw_border((win_h - h) / 2, (win_w - w) / 2, h - 1, w - 1);
}

void draw_pixel(int y, int x, bool is_on) {
    const char *pixel = (is_on) ? PIXEL_ON : PIXEL_OFF;
    mvaddstr((win_h - REAL_HEIGHT) / 2 + y, (win_w - REAL_WIDTH) / 2 + x * 2,
             pixel);
}

void draw_pixel_hi_res(int y, int x, bool is_on) {
    int start_x = (win_w - REAL_WIDTH) / 2;
    int start_y = (win_h - REAL_HEIGHT) / 2;
    unsigned short codepoint = mvinch(start_y + y / 2, start_x + x);

    /* utf8 encoded block element characters
     * (with the added space)
     * ▀: {0xe2, 0x96, 0x80, 0}
     * █: {0xe2, 0x96, 0x88, 0}
     * ▄: {0xe2, 0x96, 0x84, 0}
     *  : {0x20, 0, *, *} (space)
     */
    char pixel[] = {0xe2, 0x96, codepoint & 0x00ff, 0};

    if (codepoint == ' ') {
        if (!is_on) return;
        pixel[2] = 0x80 + (y % 2) * 4;
    }

    switch (codepoint & 0x00ff) {
        case 0x80:
        case 0x84:
            // is the pixel currently ▀ or ▄
            bool is_current_top = (codepoint & 0x00ff) == 0x80;
            // should the top half or the bottom half be changed
            bool is_changing_top = y % 2 == 0;
            // if the pixel is already in the right state, do nothing
            if ((is_changing_top == is_current_top) == is_on) return;
            // turn off the other half
            if ((is_changing_top == is_current_top) && !is_on) {
                pixel[0] = 0x20;
                pixel[1] = '\0';
                break;
            }
            // turn on the other half
            pixel[2] = 0x88;
            break;
        case 0x88:
            if (is_on) return;
            pixel[2] = 0x80 + ((y + 1) % 2) * 4;
            break;
    }
    mvaddstr(start_y + y / 2, start_x + x, pixel);
}

void display_small_window_message() {
    clear();
    draw_centered_border(3, strlen(SMALL_WINDOW_MESSAGE) + 2);
    mvaddstr((win_h - 1) / 2, (win_w - strlen(SMALL_WINDOW_MESSAGE)) / 2,
             SMALL_WINDOW_MESSAGE);
    refresh();
}

void set_win_dimens() {
    struct winsize ws;
    ioctl(0, TIOCGWINSZ, &ws);
    win_w = ws.ws_col;
    win_h = ws.ws_row;
}

void handle_win_size(unsigned char *video_mem, bool hi_res) {
    static int last_win_h, last_win_w;
    set_win_dimens();
    bool is_win_small = last_win_w != win_w || last_win_h != win_h;
    if (is_win_small) {
        draw_all(video_mem, hi_res);
        last_win_w = win_w;
        last_win_h = win_h;
    }
    while (win_h < REAL_HEIGHT || win_w < REAL_WIDTH) {
        if (is_win_small) {
            display_small_window_message();
            last_win_w = win_w;
            last_win_h = win_h;
        }
        set_win_dimens();
        usleep(0.5 * SECONDS);
    }
}

void init_graphics() {
    setlocale(LC_CTYPE, "");
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, true);
    curs_set(0);
    set_win_dimens();
    DRAW_BORDER();
    refresh();
}

void clear_screen() {
    clear();
    DRAW_BORDER();
    refresh();
}

void draw(unsigned char *video_mem, unsigned int video_signal, bool hi_res) {
    unsigned short xy = GET_XY(video_signal);
    unsigned char n = GET_N(video_signal);
    int x = xy % NUM_BYTES_IN_ROW;
    int y = xy / NUM_BYTES_IN_ROW;
    if (n == 0) n = 16;
    int height = (hi_res) ? HEIGTH : HEIGTH / 2;
    int width = (hi_res) ? WIDTH : WIDTH / 2;

    for (int i = 0; i < n && y + i < height; i++) {
        // Get 16 or 24 pixels from (y + i, x) to (y + i, x + 16 or x + 24)
        unsigned char *p = &video_mem[x + (y + i) * NUM_BYTES_IN_ROW];
        unsigned int curr_int = 0;
        for (int j = 0; j < 3 && p + j < video_mem + SIZE_VIDEO_MEM; j++) {
            curr_int |= p[j] << (3 - j) * 8;
        }

        // Draw those pixels
        for (int j = 0; j < 3 * 8 && x * 8 + j < width; j++) {
            if (hi_res)
                draw_pixel_hi_res(y + i, x * 8 + j, (curr_int >> 31));
            else
                draw_pixel(y + i, x * 8 + j, (curr_int >> 31));
            curr_int <<= 1;
        }
    }
    refresh();
}

void draw_all(unsigned char *video_mem, bool hi_res) {
    clear_screen();
    for (int num_byte = 0; num_byte < SIZE_VIDEO_MEM; num_byte++) {
        unsigned char curr_byte = video_mem[num_byte];
        int x = num_byte % NUM_BYTES_IN_ROW;
        int y = num_byte / NUM_BYTES_IN_ROW;
        if (!hi_res && x >= NUM_BYTES_IN_ROW / 2) continue;
        if (!hi_res && y >= HEIGTH / 2) break;
        for (int i = 0; i < 8; i++) {
            if (hi_res) {
                draw_pixel_hi_res(y, x * 8 + i, (curr_byte >> 7));
                curr_byte <<= 1;
                continue;
            }
            draw_pixel(y, x * 8 + i, (curr_byte >> 7));
            curr_byte <<= 1;
        }
    }
}

void st_flash(bool is_pixel_on) {
    static bool was_pixel_on = false;
    if (was_pixel_on == is_pixel_on) {
        return;
    }
    was_pixel_on = is_pixel_on;

    char pixel = (is_pixel_on) ? '@' : ' ';
    int flash_h = (win_h - (REAL_HEIGHT + 2)) / 2;
    char *pixels = malloc(win_w * flash_h + 1);
    memset(pixels, pixel, win_w * flash_h);
    pixels[win_w * flash_h + 1] = '\0';
    mvaddstr(0, 0, pixels);
    mvaddstr((win_h + REAL_HEIGHT) / 2 + 1, 0, pixels);
    free(pixels);

    int flash_w = (win_w - REAL_WIDTH) / 2;
    pixels = malloc(flash_w);
    memset(pixels, pixel, flash_w - 1);
    pixels[flash_w - 1] = '\0';
    for (int i = 0; i <= REAL_HEIGHT + 1; i++) {
        mvaddstr(flash_h + i, 0, pixels);
        mvaddstr(flash_h + i, (win_w + REAL_WIDTH) / 2 + 1, pixels);
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
    int y = (win_h - REAL_HEIGHT) / 4;
    int x = (win_w - strlen(XSET_MESSAGE)) / 2;
    draw_border(y - 1, x - 1, 2, strlen(XSET_MESSAGE) + 1);
    mvaddstr(y, x, XSET_MESSAGE);
    refresh();
}

void clear_xset_message() {
    int y = (win_h - REAL_HEIGHT) / 4 - 1;
    int x = (win_w - strlen(XSET_MESSAGE)) / 2 - 1;
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
