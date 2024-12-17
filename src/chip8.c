#include <stdio.h>

#define PROGRAM_START 0x200
#define STACK_END 0xea0
#define STACK_START 0xeff

typedef void (*instruction)(unsigned short);

unsigned char memory[4096];
unsigned short *stack = (unsigned short *)memory + STACK_END + 0xff;
unsigned char V[16];
unsigned short pc;
unsigned char sp;
unsigned char clock;

void init_chip8() {
	pc = PROGRAM_START;
	sp = 0xff;
	clock = 0;
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

unsigned short fetch() {
	unsigned short opcode;
	opcode = memory[pc] << 8;
	opcode += memory[pc + 1];
	pc += 2;
	return opcode;
}

void load(unsigned short opcode) {
	char register_num = (opcode & 0x0100) >> 8;
	char value = opcode & 0xff;
	V[(int)register_num] = value;
	printf("EXECUTED: LD V%x, %x\n", (int)register_num, (int)value);
}

instruction decode(unsigned short opcode) {
	if (opcode & 0x6000) {
		printf("DECODED: LD\n");
		return &load;
	}
	return NULL;
}

int next_cycle() {
	unsigned static short opcode;
	instruction static inst;
	if (inst == NULL && clock == 2) {
		printf("Ilegal opcode\n");
		return 1;
	}
	switch (clock) {
		case 0:
			opcode = fetch();
			printf("FETCHED: %04x\n", opcode);
			break;
		case 1:
			inst = decode(opcode);
			break;
		case 2:
			inst(opcode);
			break;
	}
	clock++;
	clock %= 3;
	return 0;
}
