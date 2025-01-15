#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chip8.h"

#define SIZE_MEMORY (START_VIDEO_MEM + SIZE_VIDEO_MEM)
#define FIRST(op) (op >> 12)

#define ADDR(op) (op & 0x0fff)

typedef struct {
    bool is_last_inst;
    char inst[5];
    char first_arg[5];
    char second_arg[5];
    char third_arg[2];
} AsmInst;

AsmInst get_last_inst() {
    AsmInst asm_inst;
    memset(&asm_inst, 0, sizeof(AsmInst));
    asm_inst.is_last_inst = true;
    return asm_inst;
}

void asm_inst_to_str(char *dest, AsmInst src) {
    if (strlen(src.third_arg) != 0) {
        sprintf(dest, "%-4s %-4s, %-4s, %-1s", src.inst, src.first_arg,
                src.second_arg, src.third_arg);
        return;
    }
    if (strlen(src.second_arg) != 0) {
        sprintf(dest, "%-4s %-4s, %-4s", src.inst, src.first_arg,
                src.second_arg);
        return;
    }
    if (strlen(src.first_arg) != 0) {
        sprintf(dest, "%-4s %-4s", src.inst, src.first_arg);
        return;
    }
    sprintf(dest, "%-4s", src.inst);
}

AsmInst decode(unsigned short opcode) {
    AsmInst asm_inst;
    memset(&asm_inst, 0, sizeof(AsmInst));

    switch (FIRST(opcode)) {
        case 1:
            strcpy(asm_inst.inst, "JP");
            sprintf(asm_inst.first_arg, "%x", ADDR(opcode));
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
        unsigned short opcode = (buffer[i] << 8) | buffer[i + 1];
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
