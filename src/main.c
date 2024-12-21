#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include "chip8.h"
#include "debugger.h"
#include "graphics.h"

unsigned long get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}

unsigned char translate(char key) {
	if (key == ERR) return KEYBOARD_UNSET;
	switch (key) {
		case '1': return 0x1;
		case '2': return 0x2;
		case '3': return 0x3;
		case '4': return 0xc;
		case 'q': return 0x4;
		case 'w': return 0x5;
		case 'e': return 0x6;
		case 'r': return 0xd;
		case 'a': return 0x7;
		case 's': return 0x8;
		case 'd': return 0x9;
		case 'f': return 0xe;
		case 'z': return 0xa;
		case 'x': return 0x0;
		case 'c': return 0xb;
		case 'v': return 0xf;
	}
	return KEYBOARD_UNSET;
}

// NOTE: Must change the repeat delay for responsiveness
//	 "xset r on rate <rate>"
unsigned char get_key(bool *is_key_pressed, Flag flag) {
	if (flag == KEYBOARD_NONBLOCKING) {
		for (int i = 0; i < 16; i++) {
			if (is_key_pressed[i]) return i;
		}
		return KEYBOARD_UNSET;
	}
	unsigned char key = KEYBOARD_UNSET;
	while (key == KEYBOARD_UNSET) {
		key = translate(getch());
	}
	return key;
}

void update_keys(bool *keys) {
	char key = getch();
	for (int i = 0; i < 16; i++) {
		keys[i] = translate(key) == i;
	}
}

void update_timers(bool *keys) {
	unsigned static long cpu_timers;
	unsigned static long keyboard_timer;
	unsigned long now = get_time();
	// NOTE: This is NOT the optimal Hz for responsive input
	if (now - keyboard_timer >= 1000000 / 40) {
		update_keys(keys);
		keyboard_timer = now;
	}

	if (now - cpu_timers >= 1000000 / 60) {
		Flag timer_flag = decrement_timers();
		if (timer_flag == SOUND) st_flash(true);
		else st_flash(false);
		cpu_timers = now;
	}

}

int main(int argc, char *argv[]) {
	int status;
	if (argc != 2 && argc != 3) {
		printf("Usage: ./chip8_emu [-d] <program_path>\n");
		return 1;
	}
	if (argc == 3 && strcmp(argv[1], "-d") != 0) {
		printf("Usage: ./chip8_emu [-d] <program_path>\n");
		return 1;
	}
	
	const char *program_path = argv[1];
	if (argc == 3) {
		set_debug();
		program_path = argv[2];
	}

	// For RND instruction
	srand(time(NULL));

	init_chip8();
	status = load_program(program_path);
	if (status == 1) {
		printf("Exiting...\n");
		return 1;
	}

	if (!should_debug()) {
		init_graphics();
	}

	bool is_key_pressed[16];
	while (1) {
		if (should_debug()) {
			printf("\e[1;1H\e[2J");
			next_cycle();
			printf("\n");
			print_state();
			fgetc(stdin);
			decrement_timers();
			continue;
		}
		Flag flag = next_cycle();
		if (flag == DRAW) draw(get_video_mem());
		if (flag == KEYBOARD_BLOCKING) {
			load_key(KEYBOARD_UNSET, get_key(is_key_pressed, KEYBOARD_BLOCKING));
		}
		if (flag == KEYBOARD_NONBLOCKING) {
			skip_key(KEYBOARD_UNSET, KEYBOARD_UNSET, get_key(is_key_pressed, KEYBOARD_NONBLOCKING));
		}
		update_timers(is_key_pressed);
		usleep(1);
	}
	endwin();
	return 0;
}
