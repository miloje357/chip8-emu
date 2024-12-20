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

	unsigned long start_time = get_time();
	unsigned long end_time;
	while (1) {
		if (should_debug()) {
			printf("\e[1;1H\e[2J");
			next_cycle();
			printf("\n");
			print_state();
			fgetc(stdin);
			update_timers();
			continue;
		}
		Flag flag = next_cycle();
		if (flag == DRAW) draw(get_video_mem());
		usleep(1);
		end_time = get_time();
		if (end_time - start_time >= 1000000 / 60) {
			Flag timer_flag = update_timers();
			if (timer_flag == SOUND) st_flash(true);
			else st_flash(false);
			start_time = end_time;
		}
	}

	// TODO: Run endwin somewhere
	return 0;
}
