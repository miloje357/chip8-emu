#include <locale.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PIXEL_ON "██"
#define PIXEL_OFF "  "

#define WIDHT 64
#define HEIGHT 32
#define DRAW_BORDER() draw_centered_border(HEIGHT + 1, WIDHT * 2 + 1)

void draw_border(int y, int x, int h, int w)
{
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

void draw(unsigned char *video_mem) {
	for (int i = 0; i < 0x100; i++) {
		unsigned char curr_byte = video_mem[i];
		for (int j = 0; j < 8; j++) {
			int x = (i % 8) * 8 + j;
			int y = i / 8;
			const char *pixel = (curr_byte & 0x80) ? PIXEL_ON : PIXEL_OFF;
			draw_pixel(y, x, pixel);
			curr_byte <<= 1;
		}
	}
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
