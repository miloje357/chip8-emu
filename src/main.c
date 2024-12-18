#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "chip8.h"
#include "debugger.h"
#include "graphics.h"

#define CLOCK_CYCLE 1.0 // in MHz

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
	if (!should_debug()) {
		init_graphics();
	}

	status = load_program(program_path);
	if (status == 1) {
		printf("Exiting...\n");
		return 1;
	}

	while (1) {
		if (should_debug()) {
			printf("\e[1;1H\e[2J");
			next_cycle();
			printf("\n");
			print_state();
			fgetc(stdin);
			continue;
		}
		next_cycle();
		usleep(1.0 / (CLOCK_CYCLE * 1000000));
	}

	// TODO: Run endwin somewhere
	return 0;
}
