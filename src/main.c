#include <stdio.h>
#include <unistd.h>
#include "chip8.h"

#define CLOCK_CYCLE 1.0 // in MHz

int main(int argc, char *argv[]) {
	int status;
	if (argc != 2) {
		printf("Usage: ./chip8_emu <program_path>\n");
		return 1;
	}

	init_chip8();
	// Init graphics
	status = load_program(argv[1]);
	if (status == 1) {
		printf("Exiting...\n");
		return 1;
	}

	while (1) {
		next_cycle();
		printf("\n");
		// Display graphics
		print_state();
		fgetc(stdin);
		// usleep(1.0 / (CLOCK_CYCLE * 1000000));
	}
	return 0;
}
