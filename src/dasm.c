#include "dasm.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chip8.h"

#define MIN(a, b) ((a < b) ? a : b)
#define DOES_STR_EXIST(str) (strlen(str) != 0)

#define SIZE_MEMORY (START_VIDEO_MEM + SIZE_VIDEO_MEM)

#define FIRST(op) (op & 0x000f)
#define SECOND(op) ((op & 0x00f0) >> 4)
#define THIRD(op) ((op & 0x0f00) >> 8)
#define FOURTH(op) (op >> 12)
#define FIRST_HALF(op) (op & 0x00ff)

#define IMMEDIATE(op) (op & 0x00ff)
#define ADDR(op) (op & 0x0fff)

#define VX_ARG "VX_ARG"
#define VY_ARG "VY_ARG"
#define N_ARG "N_ARG"
#define ADDR_ARG "ADDR_ARG"
#define IMM_ARG "IMM_ARG"

#define ZERO_ARG_INST(inst, inst_name) strcpy(inst->name, inst_name)
#define ONE_ARG_INST(inst, opcode, inst_name, arg1) \
    {                                               \
        strcpy(inst->name, inst_name);              \
        append_arg(inst, arg1, opcode);             \
    }
#define TWO_ARG_INST(inst, opcode, inst_name, arg1, arg2) \
    {                                                     \
        strcpy(inst->name, inst_name);                    \
        append_arg(inst, arg1, opcode);                   \
        append_arg(inst, arg2, opcode);                   \
    }
#define THREE_ARG_INST(inst, opcode, inst_name, arg1, arg2, arg3) \
    {                                                             \
        strcpy(inst->name, inst_name);                            \
        append_arg(inst, arg1, opcode);                           \
        append_arg(inst, arg2, opcode);                           \
        append_arg(inst, arg3, opcode);                           \
    }

void append_arg(AsmStatement *inst, const char *arg, unsigned short opcode) {
    char *dest = inst->args[inst->num_args];
    inst->num_args++;
    if (strcmp(arg, VX_ARG) == 0) {
        sprintf(dest, "V%X", THIRD(opcode));
        return;
    }
    if (strcmp(arg, VY_ARG) == 0) {
        sprintf(dest, "V%X", SECOND(opcode));
        return;
    }
    if (strcmp(arg, N_ARG) == 0) {
        sprintf(dest, "%d", FIRST(opcode));
        return;
    }
    if (strcmp(arg, ADDR_ARG) == 0) {
        sprintf(dest, "L%03X", ADDR(opcode));
        return;
    }
    if (strcmp(arg, IMM_ARG) == 0) {
        sprintf(dest, "%d", IMMEDIATE(opcode));
        return;
    }
    strcpy(dest, arg);
}

void decode_zero(AsmStatement *inst, unsigned short opcode) {
    static const char *inst_names[0x100] = {
        [0xe0] = "CLS",  [0xee] = "RET", [0xfb] = "SCR", [0xfc] = "SCL",
        [0xfd] = "EXIT", [0xfe] = "LOW", [0xff] = "HIGH"};
    const char *inst_name = inst_names[FIRST_HALF(opcode)];
    if (inst_name == NULL) return;
    // All 0xxx instructions have no arguments and are of the form 00xx
    ZERO_ARG_INST(inst, inst_name);
}

void decode_eight(AsmStatement *inst, unsigned short opcode) {
    static const char *inst_names[0x10] = {
        "LD", "OR", "AND", "XOR", "ADD", "SUB", "SHR", "SUBN", [0xe] = "SHL"};
    const char *inst_name = inst_names[FIRST(opcode)];
    if (inst_name == NULL) return;
    // All 8xxx instructions have two registers as arguments
    TWO_ARG_INST(inst, opcode, inst_name, VX_ARG, VY_ARG);
}

void decode_e(AsmStatement *inst, unsigned short opcode) {
    switch (FIRST_HALF(opcode)) {
        case 0x9e:
            ONE_ARG_INST(inst, opcode, "SKP", VX_ARG);
            break;
        case 0xa1:
            ONE_ARG_INST(inst, opcode, "SKNP", VX_ARG);
            break;
    }
}

void decode_f(AsmStatement *inst, unsigned short opcode) {
    static const char *inst_args[0x100] = {
        [0x07] = "DT",  [0x0a] = "K", [0x15] = "DT", [0x18] = "ST",
        [0x1e] = "I",   [0x29] = "F", [0x30] = "HF", [0x55] = "[I]",
        [0x65] = "[I]", [0x75] = "R", [0x85] = "R"};

    if (FIRST_HALF(opcode) == 0x33) {
        ONE_ARG_INST(inst, opcode, "BCD", VX_ARG);
        return;
    }

    const char *arg = inst_args[FIRST_HALF(opcode)];
    if (arg == NULL) return;

    switch (FIRST_HALF(opcode)) {
        case 0x07:
        case 0x0a:
        case 0x65:
        case 0x85:
            // These instructions are of the form LD Vx, **
            TWO_ARG_INST(inst, opcode, "LD", VX_ARG, arg);
            break;
        case 0x15:
        case 0x18:
        case 0x29:
        case 0x30:
        case 0x55:
        case 0x75:
            // These instructions are of the form LD **, Vx
            TWO_ARG_INST(inst, opcode, "LD", arg, VX_ARG);
            break;
        case 0x1e:
            // ADD I, Vx
            TWO_ARG_INST(inst, opcode, "ADD", arg, VX_ARG);
            break;
    }
}

// Decodes the opcode to an instruction, and puts it in the dest
void decode_dasm(AsmStatement *inst, unsigned short opcode, bool has_quirks) {
    switch (FOURTH(opcode)) {
        case 0:
            decode_zero(inst, opcode);
            break;
        case 1:
            ONE_ARG_INST(inst, opcode, "JP", ADDR_ARG);
            break;
        case 2:
            ONE_ARG_INST(inst, opcode, "CALL", ADDR_ARG);
            break;
        case 3:
            TWO_ARG_INST(inst, opcode, "SE", VX_ARG, IMM_ARG);
            break;
        case 4:
            TWO_ARG_INST(inst, opcode, "SNE", VX_ARG, IMM_ARG);
            break;
        case 5:
            TWO_ARG_INST(inst, opcode, "SE", VX_ARG, VY_ARG);
            break;
        case 6:
            TWO_ARG_INST(inst, opcode, "LD", VX_ARG, IMM_ARG);
            break;
        case 7:
            TWO_ARG_INST(inst, opcode, "ADD", VX_ARG, IMM_ARG);
            break;
        case 8:
            decode_eight(inst, opcode);
            break;
        case 9:
            TWO_ARG_INST(inst, opcode, "SNE", VX_ARG, VY_ARG);
            break;
        case 0xa:
            TWO_ARG_INST(inst, opcode, "LD", "I", ADDR_ARG);
            break;
        case 0xb:
            if (has_quirks) {
                TWO_ARG_INST(inst, opcode, "JP", VX_ARG, ADDR_ARG);
                break;
            }
            TWO_ARG_INST(inst, opcode, "JP", "V0", ADDR_ARG);
            break;
        case 0xc:
            TWO_ARG_INST(inst, opcode, "RND", VX_ARG, IMM_ARG);
            break;
        case 0xd:
            THREE_ARG_INST(inst, opcode, "DRW", VX_ARG, VY_ARG, N_ARG);
            break;
        case 0xe:
            decode_e(inst, opcode);
            break;
        case 0xf:
            decode_f(inst, opcode);
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
        (FIRST_HALF(opcode) == 0xa1 || FIRST_HALF(opcode) == 0x9e)) {
        // check pc + 2, then returns to pc + 2
        set_is_reachable(dest, bytes, len, pc + 4);
    }

    // go to next opcode
    // TODO: Rewrite without unnecessary recursion
    set_is_reachable(dest, bytes, len, pc + 2);
}

bool should_put_label(AsmStatement inst) {
    return strcmp(inst.name, "JP") == 0 || strcmp(inst.name, "CALL") == 0 ||
           (strcmp(inst.name, "LD") == 0 && strcmp(inst.args[0], "I") == 0);
}

int count_reachable(bool *src, size_t len) {
    int count = 0;
    for (size_t i = 0; i < len; i++) {
        count += src[i];
    }
    return count;
}

AsmStatement *filter_empty(AsmStatement *src, size_t *num_statements) {
    AsmStatement *filtered_assembly =
        malloc(sizeof(AsmStatement) * *num_statements);
    size_t count = 0;
    for (size_t i = 0; i < *num_statements; i++) {
        if (DOES_STR_EXIST(src[i].name)) {
            filtered_assembly[count++] = src[i];
        }
    }
    *num_statements = count;
    filtered_assembly =
        realloc(filtered_assembly, sizeof(AsmStatement) * *num_statements);
    free(src);
    return filtered_assembly;
}

// NOTE: Dissasembles SIZE_MEMORY bytes (currently it's 4864)
AsmStatement *disassemble(FILE *program_file, size_t *num_statements,
                          bool has_quirks) {
    unsigned char bytes[SIZE_MEMORY];
    size_t bytes_read = fread(bytes, 1, SIZE_MEMORY, program_file);
    if (bytes_read == 0) {
        printf("Couldn't read file.\n");
        return NULL;
    }

    bool is_reachable[bytes_read];
    memset(is_reachable, false, bytes_read);
    set_is_reachable(is_reachable, bytes, bytes_read, 0);
    int num_reachable = count_reachable(is_reachable, bytes_read);
    *num_statements = (num_reachable / 2) + (bytes_read - num_reachable);
    AsmStatement *assembly = malloc(sizeof(AsmStatement) * *num_statements);
    memset(assembly, 0, sizeof(AsmStatement) * *num_statements);

    // Decode instructions
    int unreachable_count = 0;
    for (int i = 0; i < bytes_read; i++) {
        if (!is_reachable[i]) {
            // This is not an instruction
            unreachable_count++;
            continue;
        }

        // This is an instruction
        AsmStatement *curr_inst = &assembly[(i + unreachable_count) / 2];
        unsigned short opcode = (bytes[i] << 8) | bytes[i + 1];
        decode_dasm(curr_inst, opcode, has_quirks);
        i++;  // Instructions occupy two bytes

        // Set lable
        if (should_put_label(*curr_inst)) {
            unsigned short addr = ADDR(opcode) - 0x200;
            addr -= count_reachable(is_reachable, MIN(addr, bytes_read)) / 2;
            if (addr >= *num_statements) continue;
            AsmStatement *label_asm_stat = &assembly[addr];
            sprintf(label_asm_stat->label, "L%03X", ADDR(opcode));
        }
    }

    // Set directives (.db)
    AsmStatement *curr_dir = NULL;
    int reachable_count = 0;
    for (int i = 0; i < bytes_read; i++) {
        if (is_reachable[i]) {
            // This is not a direcitve, move on
            reachable_count++;
            curr_dir = NULL;
            continue;
        }

        // This is a directive
        if (curr_dir == NULL || curr_dir->num_args == 4 ||
            DOES_STR_EXIST(assembly[i - reachable_count / 2].label)) {
            // This is a new directive
            curr_dir = &assembly[i - reachable_count / 2];
            if (!DOES_STR_EXIST(curr_dir->label)) {
                // There was no label prior to this
                sprintf(curr_dir->label, "L%03X", (0x200 + i) & 0xfff);
            }
            strcpy(curr_dir->name, ".db");
            curr_dir->is_directive = true;
        }
        sprintf(curr_dir->args[curr_dir->num_args], "0x%02X", bytes[i]);
        curr_dir->num_args++;
    }

    return filter_empty(assembly, num_statements);
}
