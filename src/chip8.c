#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "debugger.h"

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define PROGRAM_START 0x200
#define STACK_START 0xedf
#define STACK_END 0xeff
#define FIRST_DEG(opcode) opcode & 0x000f
#define SECOND_DEG(opcode) (opcode & 0x00f0) >> 4
#define THIRD_DEG(opcode) (opcode & 0x0f00) >> 8
#define FOURTH_DEG(opcode) (opcode & 0xf000) >> 12
#define IMMEDIATE(opcode) opcode & 0x00ff
#define ADDR(opcode) opcode & 0x0fff

typedef void (*instruction)(unsigned short);

unsigned char memory[4096];
unsigned char V[16];
unsigned short pc;
unsigned char sp;
unsigned char clock;
unsigned short I;

void init_chip8() {
	pc = PROGRAM_START;
	sp = 0;
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
	debug_printf("Loaded %s\n", program_path);
	return 0;
}

void print_state() {
	printf("Registers:       ");
	for (int i = 0; i < 16; i++) {
		printf("%02x   ", V[i]);
	}
	printf("\n");

	printf("Stack:           ");
	for (int i = STACK_START + 2; i <= STACK_END; i += 2) {
		if (i - STACK_START - 2 == sp) {
			printf(ANSI_COLOR_RED "%04x " ANSI_COLOR_RESET, *(unsigned short *)(memory + i));
			continue;
		}
		printf("%04x ", *(unsigned short *)(memory + i));
	}
	printf("\n");

	printf("Stack pointer:   %02x\n", sp);
	printf("Program counter: %04x\n", pc);
	printf("Index register:  %04x\n", I);
	printf("\n");

	for (int i = 0; i < 32; i++) {
		printf("           %04x  ", PROGRAM_START + i * 16);
		for (int j = 0; j < 16; j++) {
			unsigned short index = PROGRAM_START + i * 16 + j;
			if (pc - 2 == index || pc - 1 == index) {
				printf(ANSI_COLOR_RED "%02x " ANSI_COLOR_RESET, memory[index]);
			}
			else printf("%02x ", memory[index]);
			if (j == 7) printf(" ");
		}
		printf("\n");
	}
}

unsigned short fetch() {
	unsigned short opcode;
	opcode = memory[pc] << 8;
	opcode += memory[pc + 1];
	pc += 2;
	return opcode;
}

void return_op(unsigned short opcode) {
	pc = *(unsigned short *)(memory + STACK_START + sp);
	sp -= 2;
	debug_printf("EXECUTED: RET\n");
}

void jump(unsigned short opcode) {
	pc = ADDR(opcode);
	debug_printf("EXECUTED: JP %04x\n", pc);
}

void call(unsigned short opcode) {
	sp += 2;
	*(unsigned short *)(memory + STACK_START + sp) = pc;
	pc = ADDR(opcode);
	debug_printf("EXECUTED: CALL %04x\n", pc);
}

void skip_equal_immediate(unsigned short opcode) {
	if (V[(int)THIRD_DEG(opcode)] == (IMMEDIATE(opcode))) {
		pc += 2;
	}
	debug_printf("EXECUTED: SE V%x, %x\n", THIRD_DEG(opcode), IMMEDIATE(opcode));
}

void skip_not_equal_immediate(unsigned short opcode) {
	if (V[(int)THIRD_DEG(opcode)] != (IMMEDIATE(opcode))) {
		pc += 2;
	}
	debug_printf("EXECUTED: SNE V%x, %x\n", THIRD_DEG(opcode), IMMEDIATE(opcode));
}

void skip_equal_reg(unsigned short opcode) {
	if (V[(int)THIRD_DEG(opcode)] == V[(int)SECOND_DEG(opcode)]) {
		pc += 2;
	}
	debug_printf("EXECUTED: SE V%x, V%x\n", THIRD_DEG(opcode), SECOND_DEG(opcode));
}

void load_immediate(unsigned short opcode) {
	V[(int)THIRD_DEG(opcode)] = IMMEDIATE(opcode);
	debug_printf("EXECUTED: LD V%x, %x\n", THIRD_DEG(opcode), IMMEDIATE(opcode));
}

void add_immediate(unsigned short opcode) {
	V[(int)THIRD_DEG(opcode)] += IMMEDIATE(opcode);
	debug_printf("EXECUTED: ADD V%x, %x\n", THIRD_DEG(opcode), IMMEDIATE(opcode));
}

void load_reg(unsigned short opcode) {
	V[(int)THIRD_DEG(opcode)] = V[(int)SECOND_DEG(opcode)];
	debug_printf("EXECUTED: LD V%x, V%x\n", THIRD_DEG(opcode), SECOND_DEG(opcode));
}

void or_reg(unsigned short opcode) {
	V[(int)THIRD_DEG(opcode)] |= V[(int)SECOND_DEG(opcode)];
	debug_printf("EXECUTED: OR V%x, V%x\n", THIRD_DEG(opcode), SECOND_DEG(opcode));
}

void and_reg(unsigned short opcode) {
	V[(int)THIRD_DEG(opcode)] &= V[(int)SECOND_DEG(opcode)];
	debug_printf("EXECUTED: AND V%x, V%x\n", THIRD_DEG(opcode), SECOND_DEG(opcode));
}

void xor_reg(unsigned short opcode) {
	V[(int)THIRD_DEG(opcode)] ^= V[(int)SECOND_DEG(opcode)];
	debug_printf("EXECUTED: XOR V%x, V%x\n", THIRD_DEG(opcode), SECOND_DEG(opcode));
}

void add_reg(unsigned short opcode) {
	int sum = V[(int)THIRD_DEG(opcode)] + V[(int)SECOND_DEG(opcode)];
	if (sum > 0xff) {
		V[0xF] = 1;
	}
	V[(int)THIRD_DEG(opcode)] = sum;
	debug_printf("EXECUTED: ADD V%x, V%x\n", THIRD_DEG(opcode), SECOND_DEG(opcode));
}


void subtract_reg(unsigned short opcode) {
	int diff = V[(int)THIRD_DEG(opcode)] - V[(int)SECOND_DEG(opcode)];
	if (diff >= 0) {
		V[0xF] = 1;
	}
	V[(int)THIRD_DEG(opcode)] = diff;
	debug_printf("EXECUTED: SUB V%x, V%x\n", THIRD_DEG(opcode), SECOND_DEG(opcode));
}

void shift_right_reg(unsigned short opcode) {
	V[0xF] = V[(int)SECOND_DEG(opcode)] & 0x01;
	V[(int)THIRD_DEG(opcode)] = V[(int)SECOND_DEG(opcode)] >> 1;
	debug_printf("EXECUTED: SHR V%x, V%x\n", THIRD_DEG(opcode), SECOND_DEG(opcode));
}


void subtract_negated_reg(unsigned short opcode) {
	int diff = V[(int)THIRD_DEG(opcode)] - V[(int)SECOND_DEG(opcode)];
	if (diff < 0) {
		V[0xF] = 1;
	}
	V[(int)THIRD_DEG(opcode)] = -diff;
	debug_printf("EXECUTED: SUBN V%x, V%x\n", THIRD_DEG(opcode), SECOND_DEG(opcode));
}

void shift_left_reg(unsigned short opcode) {
	V[0xF] = (V[(int)SECOND_DEG(opcode)] & 0x80) >> 7;
	V[(int)THIRD_DEG(opcode)] = V[(int)SECOND_DEG(opcode)] << 1;
	debug_printf("EXECUTED: SHL V%x, V%x\n", THIRD_DEG(opcode), SECOND_DEG(opcode));
}

void skip_not_equal_reg(unsigned short opcode) {
	if (V[(int)THIRD_DEG(opcode)] != V[(int)SECOND_DEG(opcode)]) {
		pc += 2;
	}
	debug_printf("EXECUTED: SNE V%x, V%x\n", THIRD_DEG(opcode), SECOND_DEG(opcode));
}

void load_index(unsigned short opcode) {
	I = ADDR(opcode);
	debug_printf("EXECUTED: LD I, %04x\n", I);
}

void jump_reg(unsigned short opcode) {
	pc = (ADDR(opcode)) + V[0];
	debug_printf("EXECUTED: JP V0, %04x\n", ADDR(opcode));
}

void random_reg(unsigned short opcode) {
	V[(int)THIRD_DEG(opcode)] = (rand() % 0x0100) & IMMEDIATE(opcode);
	debug_printf("EXECUTED: RND V%x, %02x\n", THIRD_DEG(opcode), IMMEDIATE(opcode));
}

void add_index_reg(unsigned short opcode) {
	I += V[(int)THIRD_DEG(opcode)];
	debug_printf("EXECUTED: ADD I, V%x\n", THIRD_DEG(opcode));
}

void to_bcd(unsigned short opcode) {
	int val = V[(int)THIRD_DEG(opcode)];
	for (int i = 2; i >= 0; i--) {
		memory[I + i] = val % 10;
		val /= 10;
	}
	debug_printf("EXECUTED: BCD V%x\n", THIRD_DEG(opcode));
}

void regs_to_memory(unsigned short opcode) {
	for (int i = 0; i < THIRD_DEG(opcode); i++) {
		memory[I + i] = V[i];
	}
}

void memory_to_regs(unsigned short opcode) {
	for (int i = 0; i < THIRD_DEG(opcode); i++) {
		V[i] = memory[I + i];
	}
}

instruction decode8(unsigned short opcode) {
	switch (FIRST_DEG(opcode)) {
		case 0:
			debug_printf("DECODED: LD Vx, Vy\n");
			return &load_reg;
		case 1:
			debug_printf("DECODED: OR Vx, Vy\n");
			return &or_reg;
		case 2:
			debug_printf("DECODED: AND Vx, Vy\n");
			return &and_reg;
		case 3:
			debug_printf("DECODED: XOR Vx, Vy\n");
			return &xor_reg;
		case 4:
			debug_printf("DECODED: ADD Vx, Vy\n");
			return &add_reg;
		case 5:
			debug_printf("DECODED: SUB Vx, Vy\n");
			return &subtract_reg;
		case 6:
			debug_printf("DECODED: SHR Vx, Vy\n");
			return &shift_right_reg;
		case 7:
			debug_printf("DECODED: SUBN Vx, Vy\n");
			return &subtract_negated_reg;
		case 0xE:
			debug_printf("DECODED: SHL Vx, Vy\n");
			return &shift_left_reg;
	}
	return NULL;
}

instruction decodef(unsigned short opcode) {
	switch (opcode & 0x00ff) {
		case 0x1E:
			debug_printf("DECODED: ADD I, Vx\n");
			return &add_index_reg;
		case 0x33:
			debug_printf("DECODED: BCD Vx\n");
			return &to_bcd;
		case 0x55:
			debug_printf("DECODED: LD [I], Vx\n");
			return &regs_to_memory;
		case 0x65:
			debug_printf("DECODED: LD Vx, [I]\n");
			return &memory_to_regs;
	}
	return NULL;
}

instruction decode(unsigned short opcode) {
	switch (FOURTH_DEG(opcode)) {
		case 1:
			debug_printf("DECODED: JP addr\n");
			return &jump;
		case 2:
			debug_printf("DECODED: CALL addr\n");
			return &call;
		case 3:
			debug_printf("DECODED: SE Vx, byte\n");
			return &skip_equal_immediate;
		case 4:
			debug_printf("DECODED: SNE Vx, byte\n");
			return &skip_not_equal_immediate;
		case 5:
			if ((FIRST_DEG(opcode)) != 0) {
				return NULL;
			}
			debug_printf("DECODED: SE Vx, Vy\n");
			return &skip_equal_reg;
		case 6:
			debug_printf("DECODED: LD Vx, byte\n");
			return &load_immediate;
		case 7:
			debug_printf("DECODED: ADD Vx, byte\n");
			return &add_immediate;
		case 8:
			return decode8(opcode);
		case 9:
			if ((FIRST_DEG(opcode)) != 0) {
				return NULL;
			}
			debug_printf("DECODED: SNE Vx, Vy\n");
			return &skip_not_equal_reg;
		case 0xA:
			debug_printf("DECODED: LD I, addr\n");
			return &load_index;
		case 0xB:
			debug_printf("DECODED: JP V0, addr\n");
			return &jump_reg;
		case 0xC:
			debug_printf("DECODED: RND Vx, byte\n");
			return &random_reg;
		case 0xF:
			return decodef(opcode);
	}
	switch (opcode) {
		case 0x00EE:
			debug_printf("DECODED: RET\n");
			return &return_op;
	}
	debug_printf("DECODED: Illegal opcode\n");
	return NULL;
}

void next_cycle() {
	unsigned static short opcode;
	instruction static inst;
	if (inst == NULL && clock == 2) {
		debug_printf("Illegal opcode\n");
		clock++;
		clock %= 3;
		return;
	}
	switch (clock) {
		case 0:
			opcode = fetch();
			debug_printf("FETCHED: %04x\n", opcode);
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
}
