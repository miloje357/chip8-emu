/* TODO: 1. Write a chip8-context struct
 */
#include "chip8.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debugger.h"

#define GET_FROM_MEM(addr) chip8.memory[(addr) % SIZE_MEMORY]

#define FONT_HEIGTH 5
#define BIG_FONT_HEIGTH 10
#define BIG_FONT_OFFSET FONT_HEIGTH * 16

#define PIXELS_TO_SCROLL_RL 4
#define NUM_BYTES_IN_ROW (SIZE_VIDEO_MEM / HEIGTH)

#define FIRST(opcode) (opcode & 0x000f)
#define SECOND(opcode) ((opcode & 0x00f0) >> 4)
#define THIRD(opcode) ((opcode & 0x0f00) >> 8)
#define FOURTH(opcode) ((opcode & 0xf000) >> 12)
#define IMMEDIATE(opcode) (opcode & 0x00ff)
#define ADDR(opcode) (opcode & 0x0fff)

typedef unsigned int (*instruction)(unsigned short);
unsigned char fonts[] = {
    0xf0, 0x90, 0x90, 0x90, 0xf0,                                // 0
    0x20, 0x60, 0x20, 0x20, 0x70,                                // 1
    0xf0, 0x10, 0xf0, 0x80, 0xf0,                                // 2
    0xf0, 0x10, 0xf0, 0x10, 0xf0,                                // 3
    0x90, 0x90, 0xf0, 0x10, 0x10,                                // 4
    0xf0, 0x80, 0xf0, 0x10, 0xf0,                                // 5
    0xf0, 0x80, 0xf0, 0x90, 0xf0,                                // 6
    0xf0, 0x10, 0x20, 0x40, 0x40,                                // 7
    0xf0, 0x90, 0xf0, 0x90, 0xf0,                                // 8
    0xf0, 0x90, 0xf0, 0x10, 0xf0,                                // 9
    0xf0, 0x90, 0xf0, 0x90, 0x90,                                // A
    0xe0, 0x90, 0xe0, 0x90, 0xe0,                                // B
    0xf0, 0x80, 0x80, 0x80, 0xf0,                                // C
    0xe0, 0x90, 0x90, 0x90, 0xe0,                                // D
    0xf0, 0x80, 0xf0, 0x80, 0xf0,                                // E
    0xf0, 0x80, 0xf0, 0x80, 0x80,                                // F
    0x7c, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x7c, 0x00,  // big 0
    0x08, 0x18, 0x38, 0x08, 0x08, 0x08, 0x08, 0x08, 0x3c, 0x00,  // big 1
    0x7c, 0x82, 0x02, 0x02, 0x04, 0x18, 0x20, 0x40, 0xfe, 0x00,  // big 2
    0x7c, 0x82, 0x02, 0x02, 0x3c, 0x02, 0x02, 0x82, 0x7c, 0x00,  // big 3
    0x84, 0x84, 0x84, 0x84, 0xfe, 0x04, 0x04, 0x04, 0x04, 0x00,  // big 4
    0xfe, 0x80, 0x80, 0x80, 0xfc, 0x02, 0x02, 0x82, 0x7c, 0x00,  // big 5
    0x7c, 0x82, 0x80, 0x80, 0xfc, 0x82, 0x82, 0x82, 0x7c, 0x00,  // big 6
    0xfe, 0x02, 0x04, 0x08, 0x10, 0x20, 0x20, 0x20, 0x20, 0x00,  // big 7
    0x7c, 0x82, 0x82, 0x82, 0x7c, 0x82, 0x82, 0x82, 0x7c, 0x00,  // big 8
    0x7c, 0x82, 0x82, 0x82, 0x7e, 0x02, 0x02, 0x82, 0x7c, 0x00,  // big 9
    0x10, 0x28, 0x44, 0x82, 0x82, 0xfe, 0x82, 0x82, 0x82, 0x00,  // big A
    0xfc, 0x82, 0x82, 0x82, 0xfc, 0x82, 0x82, 0x82, 0xfc, 0x00,  // big B
    0x7c, 0x82, 0x80, 0x80, 0x80, 0x80, 0x80, 0x82, 0x7c, 0x00,  // big C
    0xfc, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0xfc, 0x00,  // big D
    0xfe, 0x80, 0x80, 0x80, 0xf8, 0x80, 0x80, 0x80, 0xfe, 0x00,  // big E
    0xfe, 0x80, 0x80, 0x80, 0xf8, 0x80, 0x80, 0x80, 0x80, 0x00,  // big F
};

Chip8Context chip8;

void init_chip8() {
    memcpy(chip8.memory, fonts, sizeof(fonts));
    chip8.pc = PROGRAM_START;
    chip8.sp = -2;
}

void skip_key(unsigned char reg, bool is_equal, unsigned char key) {
    unsigned static char saved_reg;
    static bool saved_is_equal;
    if (reg != KEYBOARD_UNSET && key == KEYBOARD_UNSET) {
        saved_reg = reg;
        saved_is_equal = is_equal;
        return;
    }
    if (reg == KEYBOARD_UNSET) {
        unsigned char are_equal = (saved_is_equal) ? 2 : 0;
        unsigned char are_not_equal = (saved_is_equal) ? 0 : 2;
        chip8.pc += (chip8.V[saved_reg] == key) ? are_equal : are_not_equal;
        return;
    }
}

void load_key(unsigned char reg, unsigned char key) {
    unsigned static char saved_reg;
    if (reg != KEYBOARD_UNSET && key == KEYBOARD_UNSET) {
        saved_reg = reg;
        return;
    }
    if (reg == KEYBOARD_UNSET && key != KEYBOARD_UNSET) {
        chip8.V[saved_reg] = key;
        return;
    }
}

int load_program(const char *program_path) {
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
    // NOTE: Not sure if program space and display buffer/stack should overlap
    if (filesize >= SIZE_MEMORY - PROGRAM_START) {
        printf("Program too large.\n");
        fclose(program);
        return 1;
    }
    fseek(program, 0, SEEK_SET);
    size_t bytes_read =
        fread(chip8.memory + PROGRAM_START, 1, filesize, program);
    if (bytes_read != filesize) {
        printf("Error reading file\n");
        fclose(program);
        return 1;
    }

    fclose(program);
    debug_printf("Loaded %s\n", program_path);
    return 0;
}

unsigned short fetch() {
    unsigned short opcode = GET_FROM_MEM(chip8.pc) << 8;
    opcode |= GET_FROM_MEM(chip8.pc + 1);
    chip8.pc += 2;
    return opcode;
}

unsigned int clear_op(unsigned short opcode) {
    memset(get_video_mem(), 0, SIZE_VIDEO_MEM);
    debug_printf("EXECUTED: CLS\n");
    return CLEAR;
}

unsigned int return_op(unsigned short opcode) {
    if (chip8.sp == 0xfe) return IDLE;
    chip8.pc = *(unsigned short *)(chip8.memory + STACK_START + chip8.sp);
    chip8.sp -= 2;
    debug_printf("EXECUTED: RET\n");
    return IDLE;
}

unsigned int scroll_down(unsigned short opcode) {
    unsigned char *video_mem = get_video_mem();
    unsigned char n = FIRST(opcode);
    // n /= (!hi_res) ? 2 : 1;
    memmove(video_mem + n * NUM_BYTES_IN_ROW, video_mem,
            (HEIGTH - n) * NUM_BYTES_IN_ROW);
    memset(video_mem, 0, n * NUM_BYTES_IN_ROW);
    debug_printf("EXECUTED: SCD nibble\n");
    return SCROLL;
}

unsigned int scroll_right(unsigned short opcode) {
    unsigned char *video_mem = get_video_mem();
    for (int i = NUM_BYTES_IN_ROW - 1; i > 0; i--) {
        for (int j = 0; j < HEIGTH; j++) {
            video_mem[j * NUM_BYTES_IN_ROW + i] >>= PIXELS_TO_SCROLL_RL;
            video_mem[j * NUM_BYTES_IN_ROW + i] |=
                video_mem[j * NUM_BYTES_IN_ROW + i - 1] << PIXELS_TO_SCROLL_RL;
        }
    }
    for (int j = 0; j < HEIGTH; j++) {
        video_mem[j * NUM_BYTES_IN_ROW] >>= PIXELS_TO_SCROLL_RL;
    }
    debug_printf("EXECUTED: SCR\n");
    return SCROLL;
}

unsigned int scroll_left(unsigned short opcode) {
    unsigned char *video_mem = get_video_mem();
    for (int i = 0; i < NUM_BYTES_IN_ROW - 1; i++) {
        for (int j = 0; j < HEIGTH; j++) {
            video_mem[j * NUM_BYTES_IN_ROW + i] <<= PIXELS_TO_SCROLL_RL;
            video_mem[j * NUM_BYTES_IN_ROW + i] |=
                video_mem[j * NUM_BYTES_IN_ROW + i + 1] >> PIXELS_TO_SCROLL_RL;
        }
    }
    for (int j = 1; j <= HEIGTH; j++) {
        video_mem[j * NUM_BYTES_IN_ROW - 1] <<= PIXELS_TO_SCROLL_RL;
    }
    debug_printf("EXECUTED: SCL\n");
    return SCROLL;
}

unsigned int exit_op(unsigned short opcode) {
    debug_printf("EXECUTED: EXIT\n");
    return EXIT;
}

unsigned int low_op(unsigned short opcode) {
    chip8.hi_res = false;
    debug_printf("EXECUTED: LOW\n");
    return IDLE;
}

unsigned int high_op(unsigned short opcode) {
    chip8.hi_res = true;
    debug_printf("EXECUTED: HIGH\n");
    return IDLE;
}

unsigned int jump(unsigned short opcode) {
    chip8.pc = ADDR(opcode);
    debug_printf("EXECUTED: JP %04x\n", chip8.pc);
    return IDLE;
}

unsigned int call(unsigned short opcode) {
    chip8.sp += 2;
    if (chip8.sp >= STACK_END - STACK_START) {
        set_error("Reached end of stack");
        return EXIT;
    }
    *(unsigned short *)(chip8.memory + STACK_START + chip8.sp) = chip8.pc;
    chip8.pc = ADDR(opcode);
    debug_printf("EXECUTED: CALL %04x\n", chip8.pc);
    return IDLE;
}

unsigned int skip_equal_immediate(unsigned short opcode) {
    if (chip8.V[THIRD(opcode)] == IMMEDIATE(opcode)) {
        chip8.pc += 2;
    }
    debug_printf("EXECUTED: SE V%x, %x\n", THIRD(opcode), IMMEDIATE(opcode));
    return IDLE;
}

unsigned int skip_not_equal_immediate(unsigned short opcode) {
    if (chip8.V[THIRD(opcode)] != IMMEDIATE(opcode)) {
        chip8.pc += 2;
    }
    debug_printf("EXECUTED: SNE V%x, %x\n", THIRD(opcode), IMMEDIATE(opcode));
    return IDLE;
}

unsigned int skip_equal_reg(unsigned short opcode) {
    if (chip8.V[THIRD(opcode)] == chip8.V[SECOND(opcode)]) {
        chip8.pc += 2;
    }
    debug_printf("EXECUTED: SE V%x, V%x\n", THIRD(opcode), SECOND(opcode));
    return IDLE;
}

unsigned int load_immediate(unsigned short opcode) {
    chip8.V[THIRD(opcode)] = IMMEDIATE(opcode);
    debug_printf("EXECUTED: LD V%x, %x\n", THIRD(opcode), IMMEDIATE(opcode));
    return IDLE;
}

unsigned int add_immediate(unsigned short opcode) {
    chip8.V[THIRD(opcode)] += IMMEDIATE(opcode);
    debug_printf("EXECUTED: ADD V%x, %x\n", THIRD(opcode), IMMEDIATE(opcode));
    return IDLE;
}

unsigned int load_reg(unsigned short opcode) {
    chip8.V[THIRD(opcode)] = chip8.V[SECOND(opcode)];
    debug_printf("EXECUTED: LD V%x, V%x\n", THIRD(opcode), SECOND(opcode));
    return IDLE;
}

unsigned int or_reg(unsigned short opcode) {
    chip8.V[THIRD(opcode)] |= chip8.V[SECOND(opcode)];
    if (!chip8.has_superchip8_quirks) chip8.V[0xf] = 0;
    debug_printf("EXECUTED: OR V%x, V%x\n", THIRD(opcode), SECOND(opcode));
    return IDLE;
}

unsigned int and_reg(unsigned short opcode) {
    chip8.V[THIRD(opcode)] &= chip8.V[SECOND(opcode)];
    if (!chip8.has_superchip8_quirks) chip8.V[0xf] = 0;
    debug_printf("EXECUTED: AND V%x, V%x\n", THIRD(opcode), SECOND(opcode));
    return IDLE;
}

unsigned int xor_reg(unsigned short opcode) {
    chip8.V[THIRD(opcode)] ^= chip8.V[SECOND(opcode)];
    if (!chip8.has_superchip8_quirks) chip8.V[0xf] = 0;
    debug_printf("EXECUTED: XOR V%x, V%x\n", THIRD(opcode), SECOND(opcode));
    return IDLE;
}

unsigned int add_reg(unsigned short opcode) {
    int sum = chip8.V[THIRD(opcode)] + chip8.V[SECOND(opcode)];
    chip8.V[THIRD(opcode)] = sum;
    chip8.V[0xf] = sum > 0xff;
    debug_printf("EXECUTED: ADD V%x, V%x\n", THIRD(opcode), SECOND(opcode));
    return IDLE;
}

unsigned int subtract_reg(unsigned short opcode) {
    int diff = chip8.V[THIRD(opcode)] - chip8.V[SECOND(opcode)];
    chip8.V[THIRD(opcode)] = diff;
    chip8.V[0xf] = diff >= 0;
    debug_printf("EXECUTED: SUB V%x, V%x\n", THIRD(opcode), SECOND(opcode));
    return IDLE;
}

unsigned int shift_right_reg(unsigned short opcode) {
    unsigned char vf = chip8.V[SECOND(opcode)] & 0x01;
    if (chip8.has_superchip8_quirks)
        chip8.V[SECOND(opcode)] >>= 1;
    else
        chip8.V[THIRD(opcode)] = chip8.V[SECOND(opcode)] >> 1;
    chip8.V[0xf] = vf;
    debug_printf("EXECUTED: SHR V%x, V%x\n", THIRD(opcode), SECOND(opcode));
    return IDLE;
}

unsigned int subtract_negated_reg(unsigned short opcode) {
    int diff = chip8.V[THIRD(opcode)] - chip8.V[SECOND(opcode)];
    chip8.V[THIRD(opcode)] = -diff;
    chip8.V[0xf] = diff <= 0;
    debug_printf("EXECUTED: SUBN V%x, V%x\n", THIRD(opcode), SECOND(opcode));
    return IDLE;
}

unsigned int shift_left_reg(unsigned short opcode) {
    unsigned char vf = (chip8.V[SECOND(opcode)] & 0x80) >> 7;
    if (chip8.has_superchip8_quirks)
        chip8.V[SECOND(opcode)] <<= 1;
    else
        chip8.V[THIRD(opcode)] = chip8.V[SECOND(opcode)] << 1;
    chip8.V[0xf] = vf;
    debug_printf("EXECUTED: SHL V%x, V%x\n", THIRD(opcode), SECOND(opcode));
    return IDLE;
}

unsigned int skip_not_equal_reg(unsigned short opcode) {
    if (chip8.V[THIRD(opcode)] != chip8.V[SECOND(opcode)]) {
        chip8.pc += 2;
    }
    debug_printf("EXECUTED: SNE V%x, V%x\n", THIRD(opcode), SECOND(opcode));
    return IDLE;
}

unsigned int load_index(unsigned short opcode) {
    chip8.I = ADDR(opcode);
    debug_printf("EXECUTED: LD I, %04x\n", chip8.I);
    return IDLE;
}

unsigned int jump_reg(unsigned short opcode) {
    int reg = (chip8.has_superchip8_quirks) ? THIRD(opcode) : 0;
    chip8.pc = ADDR(opcode) + chip8.V[reg];
    debug_printf("EXECUTED: JP V%x, %04x\n", reg, ADDR(opcode));
    return IDLE;
}

unsigned int random_reg(unsigned short opcode) {
    chip8.V[THIRD(opcode)] = (rand() % 0x0100) & IMMEDIATE(opcode);
    debug_printf("EXECUTED: RND V%x, %02x\n", THIRD(opcode), IMMEDIATE(opcode));
    return IDLE;
}

unsigned int draw_op(unsigned short opcode) {
    unsigned char vx = chip8.V[THIRD(opcode)];
    unsigned char n = FIRST(opcode);
    if (n == 0) n = 32;
    unsigned char *video_mem = get_video_mem();
    int width = (chip8.hi_res) ? WIDTH : WIDTH / 2;
    int height = (chip8.hi_res) ? HEIGTH : HEIGTH / 2;
    const unsigned short start_y =
        (chip8.V[SECOND(opcode)] % height) * NUM_BYTES_IN_ROW;
    const unsigned short start_x = (vx % width) / 8;
    int y = start_y;
    chip8.V[0xf] = 0;

    for (int i = 0; i < n && y < height * NUM_BYTES_IN_ROW; i++) {
        // Get a row of a sprite
        unsigned int sprite_int = GET_FROM_MEM(chip8.I + i) << 24;
        if (n == 32) {
            sprite_int |= GET_FROM_MEM(chip8.I + i + 1) << 16;
            i++;
        }
        sprite_int >>= (vx % 8);

        // Draw row
        if (y < start_y) break;
        for (int x = start_x; x < 16; x++) {
            unsigned char curr_byte = sprite_int >> 24;
            if ((video_mem[x + y] & curr_byte) != 0) {
                chip8.V[0xf] = 1;
            }
            sprite_int <<= 8;
            video_mem[x + y] ^= curr_byte;
        }
        y += NUM_BYTES_IN_ROW;
    }

    debug_printf("EXECUTED: DRW V%x, V%x, %x\n", THIRD(opcode), SECOND(opcode),
                 FIRST(opcode));
    return SET_XY(start_x + start_y) | SET_N(FIRST(opcode)) |
           ((chip8.hi_res) ? DRAW_HI_RES : DRAW);
}

unsigned int skip_key_op(unsigned short opcode) {
    skip_key(THIRD(opcode), true, KEYBOARD_UNSET);
    debug_printf("EXECUTING: SKP V%x\n", THIRD(opcode));
    return KEYBOARD_NONBLOCKING;
}

unsigned int skip_not_key_op(unsigned short opcode) {
    skip_key(THIRD(opcode), false, KEYBOARD_UNSET);
    debug_printf("EXECUTING: SKNP V%x\n", THIRD(opcode));
    return KEYBOARD_NONBLOCKING;
}

unsigned int delay_to_reg(unsigned short opcode) {
    chip8.V[THIRD(opcode)] = chip8.dt;
    debug_printf("EXECUTED: LD V%x, DT\n", THIRD(opcode));
    return IDLE;
}

unsigned int key_to_reg(unsigned short opcode) {
    load_key(THIRD(opcode), KEYBOARD_UNSET);
    debug_printf("EXECUTING: LD V%x, K\n", THIRD(opcode));
    debug_printf("Waiting for keyboard input!");
    return KEYBOARD_BLOCKING;
}

unsigned int reg_to_delay(unsigned short opcode) {
    chip8.dt = chip8.V[THIRD(opcode)];
    debug_printf("EXECUTED: LD DT, V%x\n", THIRD(opcode));
    return IDLE;
}

unsigned int reg_to_sound(unsigned short opcode) {
    chip8.st = chip8.V[THIRD(opcode)];
    debug_printf("EXECUTED: LD ST, V%x\n", THIRD(opcode));
    return IDLE;
}

unsigned int add_index_reg(unsigned short opcode) {
    chip8.I += chip8.V[THIRD(opcode)];
    debug_printf("EXECUTED: ADD I, V%x\n", THIRD(opcode));
    return IDLE;
}

unsigned int load_font(unsigned short opcode) {
    chip8.I = (chip8.V[THIRD(opcode)] & 0x0f) * FONT_HEIGTH;
    debug_printf("EXECUTED: LD F, V%x\n", THIRD(opcode));
    return IDLE;
}

unsigned int load_big_font(unsigned short opcode) {
    chip8.I =
        BIG_FONT_OFFSET + (chip8.V[THIRD(opcode)] & 0x0f) * BIG_FONT_HEIGTH;
    debug_printf("EXECUTED: LD HF, V%x\n", THIRD(opcode));
    return IDLE;
}

unsigned int to_bcd(unsigned short opcode) {
    int val = chip8.V[THIRD(opcode)];
    for (int i = 2; i >= 0; i--) {
        chip8.memory[(chip8.I + i) & 0x0fff] = val % 10;
        val /= 10;
    }
    debug_printf("EXECUTED: BCD V%x\n", THIRD(opcode));
    return IDLE;
}

unsigned int regs_to_memory(unsigned short opcode) {
    memcpy(chip8.memory + chip8.I % SIZE_MEMORY, chip8.V, THIRD(opcode) + 1);
    if (!chip8.has_superchip8_quirks) chip8.I += THIRD(opcode) + 1;
    debug_printf("EXECUTED: LD [I], V%x\n", THIRD(opcode));
    return IDLE;
}

unsigned int memory_to_regs(unsigned short opcode) {
    memcpy(chip8.V, chip8.memory + chip8.I % SIZE_MEMORY, THIRD(opcode) + 1);
    if (!chip8.has_superchip8_quirks) chip8.I += (THIRD(opcode)) + 1;
    debug_printf("EXECUTED: LD V%x, [I]\n", THIRD(opcode));
    return IDLE;
}

unsigned int regs_to_flags(unsigned short opcode) {
    memcpy(chip8.flags, chip8.V, THIRD(opcode) + 1);
    debug_printf("DECODED:  LD R, V%x\n", THIRD(opcode));
    return IDLE;
}

unsigned int flags_to_regs(unsigned short opcode) {
    memcpy(chip8.V, chip8.flags, THIRD(opcode) + 1);
    debug_printf("DECODED:  LD V%x, R\n", THIRD(opcode));
    return IDLE;
}

instruction decode8(unsigned short opcode) {
    switch (FIRST(opcode)) {
        case 0:
            debug_printf("DECODED:  LD Vx, Vy\n");
            return &load_reg;
        case 1:
            debug_printf("DECODED:  OR Vx, Vy\n");
            return &or_reg;
        case 2:
            debug_printf("DECODED:  AND Vx, Vy\n");
            return &and_reg;
        case 3:
            debug_printf("DECODED:  XOR Vx, Vy\n");
            return &xor_reg;
        case 4:
            debug_printf("DECODED:  ADD Vx, Vy\n");
            return &add_reg;
        case 5:
            debug_printf("DECODED:  SUB Vx, Vy\n");
            return &subtract_reg;
        case 6:
            debug_printf("DECODED:  SHR Vx, Vy\n");
            return &shift_right_reg;
        case 7:
            debug_printf("DECODED:  SUBN Vx, Vy\n");
            return &subtract_negated_reg;
        case 0xe:
            debug_printf("DECODED:  SHL Vx, Vy\n");
            return &shift_left_reg;
    }
    debug_printf("DECODED:  Illegal opcode\n");
    return NULL;
}

instruction decodee(unsigned short opcode) {
    switch (SECOND(opcode) << 4 | FIRST(opcode)) {
        case 0x9e:
            debug_printf("DECODED:  SKP Vx\n");
            return &skip_key_op;
        case 0xa1:
            debug_printf("DECODED:  SKNP Vx\n");
            return &skip_not_key_op;
    }
    debug_printf("DECODED:  Illegal opcode\n");
    return NULL;
}

instruction decodef(unsigned short opcode) {
    switch (SECOND(opcode) << 4 | FIRST(opcode)) {
        case 0x07:
            debug_printf("DECODED:  LD Vx, DT\n");
            return &delay_to_reg;
        case 0x0a:
            debug_printf("DECODED:  LD Vx, K\n");
            return &key_to_reg;
        case 0x15:
            debug_printf("DECODED:  LD DT, Vx\n");
            return &reg_to_delay;
        case 0x18:
            debug_printf("DECODED:  LD ST, Vx\n");
            return &reg_to_sound;
        case 0x1e:
            debug_printf("DECODED:  ADD I, Vx\n");
            return &add_index_reg;
        case 0x29:
            debug_printf("DECODED:  LD F, Vx\n");
            return &load_font;
        case 0x30:
            debug_printf("DECODED:  LD HF, Vx\n");
            return &load_big_font;
        case 0x33:
            debug_printf("DECODED:  BCD Vx\n");
            return &to_bcd;
        case 0x55:
            debug_printf("DECODED:  LD [I], Vx\n");
            return &regs_to_memory;
        case 0x65:
            debug_printf("DECODED:  LD Vx, [I]\n");
            return &memory_to_regs;
        case 0x75:
            debug_printf("DECODED:  LD R, Vx\n");
            return &regs_to_flags;
        case 0x85:
            debug_printf("DECODED:  LD Vx, R\n");
            return &flags_to_regs;
    }
    debug_printf("DECODED:  Illegal opcode\n");
    return NULL;
}

instruction decode0(unsigned short opcode) {
    switch (SECOND(opcode) << 4 | FIRST(opcode)) {
        case 0xe0:
            debug_printf("DECODED:  CLS\n");
            return &clear_op;
        case 0xee:
            debug_printf("DECODED:  RET\n");
            return &return_op;
        case 0xfb:
            debug_printf("DECODED:  SCR\n");
            return &scroll_right;
        case 0xfc:
            debug_printf("DECODED:  SCL\n");
            return &scroll_left;
        case 0xfd:
            debug_printf("DECODED:  EXIT\n");
            return &exit_op;
        case 0xfe:
            debug_printf("DECODED:  LOW\n");
            return &low_op;
        case 0xff:
            debug_printf("DECODED:  HIGH\n");
            return &high_op;
    }
    if (SECOND(opcode) == 0xc) {
        debug_printf("DECODED:  SCD nibble\n");
        return &scroll_down;
    }
    debug_printf("DECODED:  Illegal opcode\n");
    return NULL;
}

instruction decode(unsigned short opcode) {
    switch (FOURTH(opcode)) {
        case 0:
            if (THIRD(opcode) != 0) break;
            return decode0(opcode);
        case 1:
            debug_printf("DECODED:  JP addr\n");
            return &jump;
        case 2:
            debug_printf("DECODED:  CALL addr\n");
            return &call;
        case 3:
            debug_printf("DECODED:  SE Vx, byte\n");
            return &skip_equal_immediate;
        case 4:
            debug_printf("DECODED:  SNE Vx, byte\n");
            return &skip_not_equal_immediate;
        case 5:
            if (FIRST(opcode) != 0) break;
            debug_printf("DECODED:  SE Vx, Vy\n");
            return &skip_equal_reg;
        case 6:
            debug_printf("DECODED:  LD Vx, byte\n");
            return &load_immediate;
        case 7:
            debug_printf("DECODED:  ADD Vx, byte\n");
            return &add_immediate;
        case 8:
            return decode8(opcode);
        case 9:
            if (FIRST(opcode) != 0) break;
            debug_printf("DECODED:  SNE Vx, Vy\n");
            return &skip_not_equal_reg;
        case 0xa:
            debug_printf("DECODED:  LD I, addr\n");
            return &load_index;
        case 0xb:
            debug_printf("DECODED:  JP V0, addr\n");
            return &jump_reg;
        case 0xc:
            debug_printf("DECODED:  RND Vx, byte\n");
            return &random_reg;
        case 0xd:
            debug_printf("DECODED:  DRW Vx, Vy, nibble\n");
            return &draw_op;
        case 0xe:
            return decodee(opcode);
        case 0xf:
            return decodef(opcode);
    }
    debug_printf("DECODED:  Illegal opcode\n");
    return NULL;
}

unsigned int next_cycle() {
    unsigned static clock = 0;
    unsigned static short opcode;
    instruction static inst;
    unsigned int flag = IDLE;
    if (inst == NULL && clock == 2) {
        debug_printf("EXECUTED: Illegal opcode\n");
        clock++;
        clock %= 3;
        return flag;
    }
    switch (clock) {
        case 0:
            opcode = fetch();
            debug_printf("FETCHED:  %04x\n", opcode);
            break;
        case 1:
            inst = decode(opcode);
            break;
        case 2:
            flag = inst(opcode);
            break;
    }
    clock++;
    clock %= 3;
    return flag;
}

Flag decrement_timers() {
    chip8.dt -= (chip8.dt != 0) ? 1 : 0;
    chip8.st -= (chip8.st != 0) ? 1 : 0;
    if (chip8.st != 0) return SOUND;
    return IDLE;
}

unsigned char *get_video_mem() { return chip8.memory + START_VIDEO_MEM; }

void set_superchip8_quirks() { chip8.has_superchip8_quirks = true; }

bool get_hi_res() { return chip8.hi_res; }

Chip8Context *get_chip8() { return &chip8; }
