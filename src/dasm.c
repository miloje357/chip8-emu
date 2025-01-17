#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chip8.h"

#define SIZE_MEMORY (START_VIDEO_MEM + SIZE_VIDEO_MEM)

#define FIRST(op) (op & 0x000f)
#define SECOND(op) ((op & 0x00f0) >> 4)
#define THIRD(op) ((op & 0x0f00) >> 8)
#define FOURTH(op) (op >> 12)

#define IMMEDIATE(op) (op & 0x00ff)
#define ADDR(op) (op & 0x0fff)

#define VX "VX"
#define VY "VY"
#define N "N"
#define ADR "ADR"
#define IMM "IMM"

#define ZERO_ARG_INST(asm_inst, inst_op) strcpy((asm_inst).inst, inst_op)
#define ONE_ARG_INST(asm_inst, opcode, inst_op, arg1) \
    {                                                 \
        strcpy((asm_inst).inst, inst_op);             \
        set_arg((asm_inst).first_arg, arg1, opcode);  \
    }
#define TWO_ARG_INST(asm_inst, opcode, inst_op, arg1, arg2) \
    {                                                       \
        strcpy((asm_inst).inst, inst_op);                   \
        set_arg((asm_inst).first_arg, arg1, opcode);        \
        set_arg((asm_inst).second_arg, arg2, opcode);       \
    }
#define THREE_ARG_INST(asm_inst, opcode, inst_op, arg1, arg2, arg3) \
    {                                                               \
        strcpy((asm_inst).inst, inst_op);                           \
        set_arg((asm_inst).first_arg, arg1, opcode);                \
        set_arg((asm_inst).second_arg, arg2, opcode);               \
        set_arg((asm_inst).third_arg, arg3, opcode);                \
    }

typedef struct {
    bool is_last_inst;
    char inst[5];
    char first_arg[5];
    char second_arg[5];
    char third_arg[3];
} AsmInst;

AsmInst get_last_inst() {
    AsmInst asm_inst;
    memset(&asm_inst, 0, sizeof(AsmInst));
    asm_inst.is_last_inst = true;
    return asm_inst;
}

void asm_inst_to_str(char *dest, AsmInst src) {
    if (strlen(src.third_arg) != 0) {
        sprintf(dest, "%-4s %s, %s, %s", src.inst, src.first_arg,
                src.second_arg, src.third_arg);
        return;
    }
    if (strlen(src.second_arg) != 0) {
        sprintf(dest, "%-4s %s, %s", src.inst, src.first_arg, src.second_arg);
        return;
    }
    if (strlen(src.first_arg) != 0) {
        sprintf(dest, "%-4s %s", src.inst, src.first_arg);
        return;
    }
    sprintf(dest, "%-4s", src.inst);
}

void set_arg(char *dest, const char *arg, unsigned short opcode) {
    if (strcmp(arg, VX) == 0) {
        sprintf(dest, "V%X", THIRD(opcode));
        return;
    }
    if (strcmp(arg, VY) == 0) {
        sprintf(dest, "V%X", SECOND(opcode));
        return;
    }
    if (strcmp(arg, N) == 0) {
        sprintf(dest, "%d", FIRST(opcode));
        return;
    }
    if (strcmp(arg, ADR) == 0) {
        sprintf(dest, "L%03X", ADDR(opcode));
        return;
    }
    if (strcmp(arg, IMM) == 0) {
        sprintf(dest, "%d", IMMEDIATE(opcode));
        return;
    }
    strcpy(dest, arg);
}

void decode0(AsmInst *asm_inst, unsigned short opcode) {
    static const char *zero_opcodes[0x100] = {
        [0xe0] = "CLS",  [0xee] = "RET", [0xfb] = "SCR", [0xfc] = "SCL",
        [0xfd] = "EXIT", [0xfe] = "LOW", [0xff] = "HIGH"};
    const char *inst_op = zero_opcodes[SECOND(opcode) << 4 | FIRST(opcode)];
    if (inst_op == NULL) return;
    ZERO_ARG_INST(*asm_inst, inst_op);
}

void decode8(AsmInst *asm_inst, unsigned short opcode) {
    static const char *eight_opcodes[0x10] = {
        "LD", "OR", "AND", "XOR", "ADD", "SUB", "SHR", "SUBN", [0xe] = "SHL"};
    const char *inst_op = eight_opcodes[FIRST(opcode)];
    if (inst_op == NULL) return;
    TWO_ARG_INST(*asm_inst, opcode, inst_op, VX, VY);
}

void decodee(AsmInst *asm_inst, unsigned short opcode) {
    switch (SECOND(opcode) << 4 | FIRST(opcode)) {
        case 0x9e:
            ONE_ARG_INST(*asm_inst, opcode, "SKP", VX);
            break;
        case 0xa1:
            ONE_ARG_INST(*asm_inst, opcode, "SKNP", VX);
            break;
    }
}

void decodef(AsmInst *asm_inst, unsigned short opcode) {
    static const char *args[0x100] = {
        [0x07] = "DT",  [0x0a] = "K", [0x15] = "DT", [0x18] = "ST",
        [0x1e] = "I",   [0x29] = "F", [0x30] = "HF", [0x55] = "[I]",
        [0x65] = "[I]", [0x75] = "R", [0x85] = "R"};

    unsigned char second_half = SECOND(opcode) << 4 | FIRST(opcode);

    if (second_half == 0x33) {
        ONE_ARG_INST(*asm_inst, opcode, "BCD", VX);
        return;
    }

    const char *arg = args[second_half];
    if (arg == NULL) return;

    switch (second_half) {
        case 0x07:
        case 0x0a:
        case 0x65:
        case 0x85:
            TWO_ARG_INST(*asm_inst, opcode, "LD", VX, arg);
            break;
        case 0x15:
        case 0x18:
        case 0x29:
        case 0x30:
        case 0x55:
        case 0x75:
            TWO_ARG_INST(*asm_inst, opcode, "LD", arg, VX);
            break;
        case 0x1e:
            TWO_ARG_INST(*asm_inst, opcode, "ADD", arg, VX);
            break;
    }
}

AsmInst decode(unsigned short opcode) {
    AsmInst asm_inst;
    memset(&asm_inst, 0, sizeof(AsmInst));

    switch (FOURTH(opcode)) {
        case 0:
            decode0(&asm_inst, opcode);
            break;
        case 1:
            ONE_ARG_INST(asm_inst, opcode, "JP", ADR);
            break;
        case 2:
            ONE_ARG_INST(asm_inst, opcode, "CALL", ADR);
            break;
        case 3:
            TWO_ARG_INST(asm_inst, opcode, "SE", VX, IMM);
            break;
        case 4:
            TWO_ARG_INST(asm_inst, opcode, "SNE", VX, IMM);
            break;
        case 5:
            TWO_ARG_INST(asm_inst, opcode, "SE", VX, VY);
            break;
        case 6:
            TWO_ARG_INST(asm_inst, opcode, "LD", VX, IMM);
            break;
        case 7:
            TWO_ARG_INST(asm_inst, opcode, "ADD", VX, IMM);
            break;
        case 8:
            decode8(&asm_inst, opcode);
            break;
        case 9:
            TWO_ARG_INST(asm_inst, opcode, "SNE", VX, VY);
            break;
        case 0xa:
            TWO_ARG_INST(asm_inst, opcode, "LD", "I", ADR);
            break;
        case 0xb:
            // TODO: "V0"/VX : Depends on quirks
            TWO_ARG_INST(asm_inst, opcode, "JP", "V0", ADR);
            break;
        case 0xc:
            TWO_ARG_INST(asm_inst, opcode, "RND", VX, IMM);
            break;
        case 0xd:
            THREE_ARG_INST(asm_inst, opcode, "DRW", VX, VY, N);
            break;
        case 0xe:
            decodee(&asm_inst, opcode);
            break;
        case 0xf:
            decodef(&asm_inst, opcode);
            break;
    }
    return asm_inst;
}

// NOTE: Dissasembles SIZE_MEMORY bytes (currently it's 4864)
AsmInst *disassemble(FILE *program_file) {
    unsigned char buffer[SIZE_MEMORY];
    size_t bytes_read = fread(buffer, 1, SIZE_MEMORY, program_file);
    if (bytes_read == 0) {
        printf("Couldn't read file.\n");
        return NULL;
    }

    // TODO: Use jump instructions to determine what is the program and what
    //       isn't
    AsmInst *assembly = malloc(sizeof(AsmInst) * (bytes_read / 2 + 1));
    int i;
    for (i = 0; i < bytes_read / 2; i++) {
        unsigned short opcode = (buffer[2 * i] << 8) | buffer[2 * i + 1];
        assembly[i] = decode(opcode);
    }
    assembly[i] = get_last_inst();
    return assembly;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./chip8_dasm <program_path>\n");
        return 1;
    }

    FILE *program_file = fopen(argv[1], "r");
    if (program_file == NULL) {
        printf("Error loading %s.\n", argv[1]);
        printf("Exiting...");
        return 1;
    }

    AsmInst *assembly = disassemble(program_file);
    if (assembly == NULL) {
        printf("Couldn't dissasemble program. Exiting...\n");
        fclose(program_file);
        return 1;
    }
    fclose(program_file);

    for (int i = 0; !assembly[i].is_last_inst; i++) {
        // 18 (sizeof(AsmInst)) - 1 (is_last_inst) + 1 ('\0')
        char asm_inst_str[sizeof(AsmInst)];
        asm_inst_to_str(asm_inst_str, assembly[i]);
        printf("%s\n", asm_inst_str);
    }
    free(assembly);
    return 0;
}
