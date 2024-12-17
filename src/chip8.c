#include <stdio.h>

#define PROGRAM_START 0x200
#define STACK_END 0xea0
#define STACK_START 0xeff

unsigned char memory[4096];
unsigned short *stack = (unsigned short *)memory + STACK_END + 0xff;
unsigned char V[16];
unsigned short pc;
unsigned char sp;

void init_chip8() {
	pc = PROGRAM_START;
	sp = 0xff;
}

int write_byte_to_memory(char byte, short addr) {
	if (addr < PROGRAM_START) {
		printf("Error writing to address %04x\n", addr);
		return 1;
	}
	if (addr >= STACK_END) {
		printf("Error writing to address %04x\n", addr);
		return 1;
	}
	memory[addr] = byte;
	return 0;
}

int load_program(const char* program_path) {
	int status = 0;
	FILE *program = fopen(program_path, "r");
	if (program == NULL) {
		printf("Error loading %s.\n", program_path);
		return 1;
	}

	char byte_read = fgetc(program);
	short addr = PROGRAM_START;
	while (byte_read != EOF && status == 0) {
		status = write_byte_to_memory(byte_read, addr);
		byte_read = fgetc(program);
		addr++;
	}
	if (status == 1) {
		printf("Program too large!\n");
		return 1;
	}

	fclose(program);
	printf("Loaded %s\n", program_path);
	return 0;
}

void print_state() {
	printf("Registers:       ");
	for (int i = 0; i < 16; i++) {
		printf("%02x   ", V[i]);
	}
	printf("\n");

	printf("Stack:           ");
	for (int i = 15; i >= 0; i--) {
		printf("%04x ", stack[i]);
	}
	printf("\n");

	printf("Stack pointer:   %02x\n", sp);
	printf("Program counter: %04x\n", pc);
}
