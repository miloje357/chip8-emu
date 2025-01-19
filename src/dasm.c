/* TODO: 1. Add the db directive
 *       2. Do TODOs
 *       3. Refactor
 *       4. Write dasm.h with docs
 */

#include <stddef.h>
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

// TODO: Rewrite with char args[5][4] and int num_args
typedef struct {
    bool is_last_inst;
    bool is_directive;
    char inst[5];
    char first_arg[5];
    char second_arg[5];
    char third_arg[5];
    char fourth_arg[5];
    char label[5];
} AsmStatement;

// TODO: Instead of this, use num_statements from dissasemble
AsmStatement get_last_inst() {
    AsmStatement asm_inst;
    memset(&asm_inst, 0, sizeof(AsmStatement));
    asm_inst.is_last_inst = true;
    return asm_inst;
}

void asm_inst_to_str(char *dest, AsmStatement src) {
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

void decode0(AsmStatement *asm_inst, unsigned short opcode) {
    static const char *zero_opcodes[0x100] = {
        [0xe0] = "CLS",  [0xee] = "RET", [0xfb] = "SCR", [0xfc] = "SCL",
        [0xfd] = "EXIT", [0xfe] = "LOW", [0xff] = "HIGH"};
    const char *inst_op = zero_opcodes[SECOND(opcode) << 4 | FIRST(opcode)];
    if (inst_op == NULL) return;
    ZERO_ARG_INST(*asm_inst, inst_op);
}

void decode8(AsmStatement *asm_inst, unsigned short opcode) {
    static const char *eight_opcodes[0x10] = {
        "LD", "OR", "AND", "XOR", "ADD", "SUB", "SHR", "SUBN", [0xe] = "SHL"};
    const char *inst_op = eight_opcodes[FIRST(opcode)];
    if (inst_op == NULL) return;
    TWO_ARG_INST(*asm_inst, opcode, inst_op, VX, VY);
}

void decodee(AsmStatement *asm_inst, unsigned short opcode) {
    switch (SECOND(opcode) << 4 | FIRST(opcode)) {
        case 0x9e:
            ONE_ARG_INST(*asm_inst, opcode, "SKP", VX);
            break;
        case 0xa1:
            ONE_ARG_INST(*asm_inst, opcode, "SKNP", VX);
            break;
    }
}

void decodef(AsmStatement *asm_inst, unsigned short opcode) {
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

void decode(AsmStatement *dest, unsigned short opcode) {
    switch (FOURTH(opcode)) {
        case 0:
            decode0(dest, opcode);
            break;
        case 1:
            ONE_ARG_INST(*dest, opcode, "JP", ADR);
            break;
        case 2:
            ONE_ARG_INST(*dest, opcode, "CALL", ADR);
            break;
        case 3:
            TWO_ARG_INST(*dest, opcode, "SE", VX, IMM);
            break;
        case 4:
            TWO_ARG_INST(*dest, opcode, "SNE", VX, IMM);
            break;
        case 5:
            TWO_ARG_INST(*dest, opcode, "SE", VX, VY);
            break;
        case 6:
            TWO_ARG_INST(*dest, opcode, "LD", VX, IMM);
            break;
        case 7:
            TWO_ARG_INST(*dest, opcode, "ADD", VX, IMM);
            break;
        case 8:
            decode8(dest, opcode);
            break;
        case 9:
            TWO_ARG_INST(*dest, opcode, "SNE", VX, VY);
            break;
        case 0xa:
            TWO_ARG_INST(*dest, opcode, "LD", "I", ADR);
            break;
        case 0xb:
            // TODO: "V0"/VX : Depends on quirks
            TWO_ARG_INST(*dest, opcode, "JP", "V0", ADR);
            break;
        case 0xc:
            TWO_ARG_INST(*dest, opcode, "RND", VX, IMM);
            break;
        case 0xd:
            THREE_ARG_INST(*dest, opcode, "DRW", VX, VY, N);
            break;
        case 0xe:
            decodee(dest, opcode);
            break;
        case 0xf:
            decodef(dest, opcode);
            break;
    }
}

// sets the current address as reachable, then changes the program counter
// accordingly
void set_is_reachable(bool *dest, unsigned char *bytes, size_t len,
                      unsigned short pc) {
    if (pc >= len || dest[pc]) return;
    dest[pc] = true;
    dest[pc + 1] = true;
    unsigned short opcode = (bytes[pc] << 8) | bytes[pc + 1];

    // RET
    if (opcode == 0x00ee) return;

    switch (FOURTH(opcode)) {
        case 1:
            // JP addr
            set_is_reachable(dest, bytes, len, ADDR(opcode) - 0x200);
            return;
        case 2:
            // CALL addr
            set_is_reachable(dest, bytes, len, ADDR(opcode) - 0x200);
            break;
        case 3:
        case 4:
        case 5:
        case 9:
            // SE, SNE
            // check pc + 2, then returns to pc + 2
            set_is_reachable(dest, bytes, len, pc + 4);
            break;
    }
    // SKP, SKNP
    if (FOURTH(opcode) == 0xe &&
        (IMMEDIATE(opcode) == 0xa1 || IMMEDIATE(opcode) == 0x9e)) {
        // check pc + 2, then returns to pc + 2
        set_is_reachable(dest, bytes, len, pc + 4);
    }

    // go to next opcode
    // TODO: Rewrite without unnecessary recursion
    set_is_reachable(dest, bytes, len, pc + 2);
}

bool should_put_label(AsmStatement asm_inst) {
    return strcmp(asm_inst.inst, "JP") == 0 ||
           strcmp(asm_inst.inst, "CALL") == 0 ||
           (strcmp(asm_inst.inst, "LD") == 0 &&
            strcmp(asm_inst.first_arg, "I") == 0);
}

int count_reachable(bool *src, size_t len) {
    int count = 0;
    for (size_t i = 0; i < len; i++) {
        count += src[i];
    }
    return count;
}

// NOTE: Dissasembles SIZE_MEMORY bytes (currently it's 4864)
AsmStatement *disassemble(FILE *program_file) {
    unsigned char buffer[SIZE_MEMORY];
    size_t bytes_read = fread(buffer, 1, SIZE_MEMORY, program_file);
    if (bytes_read == 0) {
        printf("Couldn't read file.\n");
        return NULL;
    }

    bool is_reachable[bytes_read];
    memset(is_reachable, false, bytes_read);
    set_is_reachable(is_reachable, buffer, bytes_read, 0);
    int num_reachable = count_reachable(is_reachable, bytes_read);
    int num_statements = (num_reachable / 2) + (bytes_read - num_reachable) + 1;

    AsmStatement *assembly = malloc(sizeof(AsmStatement) * num_statements);
    memset(assembly, 0, sizeof(AsmStatement) * num_statements);
    int count_unreachable = 0;
    for (int i = 0; i < bytes_read; i++) {
        AsmStatement *curr_stat = &assembly[(i + count_unreachable) / 2];
        if (!is_reachable[i]) {
            count_unreachable++;
            continue;
        }
        unsigned short opcode = (buffer[i] << 8) | buffer[i + 1];
        decode(curr_stat, opcode);
        i++;
        if (should_put_label(*curr_stat)) {
            unsigned short addr = ADDR(opcode) - 0x200;
            if (addr >= num_statements) continue;
            AsmStatement *label_asm_stat =
                &assembly[addr - count_reachable(is_reachable, addr) / 2];
            sprintf(label_asm_stat->label, "L%03X", ADDR(opcode));
        }
    }

    AsmStatement *curr_dir = NULL;
    int count_reachable = 0;
    for (int i = 0; i < bytes_read; i++) {
        if (is_reachable[i]) {
            count_reachable++;
            curr_dir = NULL;
            continue;
        }
        if (curr_dir == NULL || strlen(curr_dir->fourth_arg) != 0 ||
            strlen(assembly[i - count_reachable / 2].label) != 0) {
            curr_dir = &assembly[i - count_reachable / 2];
            if (strlen(curr_dir->label) == 0) {
                sprintf(curr_dir->label, "L%03X", (0x200 + i) & 0xfff);
            }
            strcpy(curr_dir->inst, ".db");
            sprintf(curr_dir->first_arg, "0x%02X", buffer[i]);
            curr_dir->is_directive = true;
            continue;
        }
        if (strlen(curr_dir->second_arg) == 0) {
            sprintf(curr_dir->second_arg, "0x%02X", buffer[i]);
            continue;
        }
        if (strlen(curr_dir->third_arg) == 0) {
            sprintf(curr_dir->third_arg, "0x%02X", buffer[i]);
            continue;
        }
        if (strlen(curr_dir->fourth_arg) == 0) {
            sprintf(curr_dir->fourth_arg, "0x%02X", buffer[i]);
            continue;
        }
    }
    assembly[num_statements - 1] = get_last_inst();
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
        printf("Exiting...\n");
        return 1;
    }

    AsmStatement *assembly = disassemble(program_file);
    if (assembly == NULL) {
        printf("Couldn't dissasemble program. Exiting...\n");
        fclose(program_file);
        return 1;
    }
    fclose(program_file);

    for (int i = 0; !assembly[i].is_last_inst; i++) {
        if (strlen(assembly[i].inst) == 0) continue;
        if (assembly[i].is_directive) {
            printf("%s: %-4s %s %s %s %s\n", assembly[i].label,
                   assembly[i].inst, assembly[i].first_arg,
                   assembly[i].second_arg, assembly[i].third_arg,
                   assembly[i].fourth_arg);
            continue;
        }
        if (strlen(assembly[i].label) != 0) {
            printf("\n%s:\n", assembly[i].label);
        }
        // 23 (sizeof(AsmInst)) - 1 (is_last_inst) + 1 ('\0') + 5 (formating)
        char asm_inst_str[sizeof(AsmStatement) + 5];
        asm_inst_to_str(asm_inst_str, assembly[i]);
        printf("\t%s\n", asm_inst_str);
    }
    free(assembly);
    return 0;
}
