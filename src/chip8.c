#include <stdio.h>
#include <stdbool.h>

#define PROGRAM_START 0x200
#define STACK_END 0xea0
#define STACK_START 0xeff
#define FIRST_DEG(opcode) opcode & 0x000f
#define SECOND_DEG(opcode) (opcode & 0x00f0) >> 4
#define THIRD_DEG(opcode) (opcode & 0x0f00) >> 8
#define FOURTH_DEG(opcode) (opcode & 0xf000) >> 12
#define IMMEDIATE(opcode) opcode & 0x00ff
#define ADDR(opcode) opcode & 0x0fff

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

int load_program(const char* program_path) {
	FILE *program = fopen(program_path, "r");
	if (program == NULL) {
		printf("Error loading %s.\n", program_path);
		return 1;
	}

	fseek(program, 0, SEEK_END);
	int filesize = ftell(program);
	if (filesize == -1) {
		printf("Couldn't determine file size.\n");
		fclose(program);
		return 1;
	}
	if (filesize >= STACK_END - PROGRAM_START) {
		printf("Program too large.\n");
		fclose(program);
		return 1;
	}
	fseek(program, 0, SEEK_SET);
	size_t bytes_read = fread(memory + PROGRAM_START, 1, filesize, program);
	if (bytes_read != filesize) {
		printf("Error reading file\n");
		fclose(program);
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

void jump(unsigned short opcode) {
	pc = ADDR(opcode);
	printf("EXECUTED: JP %04x\n", pc);
}

void skip_equal_immediate(unsigned short opcode) {
	if (V[(int)THIRD_DEG(opcode)] == (IMMEDIATE(opcode))) {
		pc += 2;
	}
	printf("EXECUTED: SE V%x, %x\n", THIRD_DEG(opcode), IMMEDIATE(opcode));
}

void skip_not_equal_immediate(unsigned short opcode) {
	if (V[(int)THIRD_DEG(opcode)] != (IMMEDIATE(opcode))) {
		pc += 2;
	}
	printf("EXECUTED: SNE V%x, %x\n", THIRD_DEG(opcode), IMMEDIATE(opcode));
}

void skip_equal_reg(unsigned short opcode) {
	if (V[(int)THIRD_DEG(opcode)] == V[(int)SECOND_DEG(opcode)]) {
		pc += 2;
	}
	printf("EXECUTED: SE V%x, V%x\n", THIRD_DEG(opcode), SECOND_DEG(opcode));
}

void load_immediate(unsigned short opcode) {
	V[(int)THIRD_DEG(opcode)] = IMMEDIATE(opcode);
	printf("EXECUTED: LD V%x, %x\n", THIRD_DEG(opcode), IMMEDIATE(opcode));
}

void add_immediate(unsigned short opcode) {
	V[(int)THIRD_DEG(opcode)] += IMMEDIATE(opcode);
	printf("EXECUTED: ADD V%x, %x\n", THIRD_DEG(opcode), IMMEDIATE(opcode));
}

instruction decode(unsigned short opcode) {
	switch (FOURTH_DEG(opcode)) {
		case 1:
			printf("DECODED: JP addr\n");
			return &jump;
		case 3:
			printf("DECODED: SE Vx, byte\n");
			return &skip_equal_immediate;
		case 4:
			printf("DECODED: SNE Vx, byte\n");
			return &skip_not_equal_immediate;
		case 5:
			if ((FIRST_DEG(opcode)) != 0) {
				return NULL;
			}
			printf("DECODED: SE Vx, Vy\n");
			return &skip_equal_reg;
		case 6:
			printf("DECODED: LD Vx, byte\n");
			return &load_immediate;
		case 7:
			printf("DECODED: ADD Vx, byte\n");
			return &add_immediate;
	}
	printf("DECODED: Illegal opcode\n");
	return NULL;
}

int next_cycle() {
	unsigned static short opcode;
	instruction static inst;
	if (inst == NULL && clock == 2) {
		printf("Illegal opcode\n");
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
