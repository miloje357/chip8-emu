#include <locale.h>
#include <ncurses.h>

#define PIXEL "██"

void init_graphics() {
	setlocale(LC_CTYPE, "");
	initscr();
	noecho();
	curs_set(0);

	// A checkered pattern for testing
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			if ((i + j) % 2 == 0) {
				mvaddstr(i, j * 2, PIXEL);
			}
		}
	}
	refresh();
}
