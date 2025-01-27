#include <config.h>
#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "chip8.h"
#include "debugger.h"
#include "game_graphics.h"
#include "tui.h"

#define DEFAULT_TICK_SPEED 900
#define SECOND 1000000

unsigned long get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * SECOND + tv.tv_usec;
}

unsigned char translate(char key) {
    static const unsigned char lookup_table[256] = {
        ['1'] = 0x1, ['2'] = 0x2, ['3'] = 0x3, ['4'] = 0xc,
        ['q'] = 0x4, ['w'] = 0x5, ['e'] = 0x6, ['r'] = 0xd,
        ['a'] = 0x7, ['s'] = 0x8, ['d'] = 0x9, ['f'] = 0xe,
        ['z'] = 0xa, ['x'] = 0x0, ['c'] = 0xb, ['v'] = 0xf};
    if (key == 'x') return 0;
    if (key == ERR) return KEYBOARD_UNSET;
    return lookup_table[(unsigned char)key] ? lookup_table[(unsigned char)key]
                                            : KEYBOARD_UNSET;
}

unsigned char get_key(bool *is_key_pressed, Flag flag) {
    if (flag == KEYBOARD_NONBLOCKING) {
        for (int i = 0; i < 16; i++) {
            if (is_key_pressed[i]) return i;
        }
        return KEYBOARD_UNSET;
    }
    // KEYBOARD_BLOCKING
    unsigned char key = KEYBOARD_UNSET;
    while (key == KEYBOARD_UNSET) {
        handle_win_size(get_video_mem(), get_hi_res());
        key = translate(getch());
        // Beacuase every non-blocking keyboard signal is a blocking one in
        // graphic debugging mode, there must be a way to get the "no key
        // pressed" action
        // BUG: Means that BLOCKING can return KEYBOARD_UNSET when
        //      GRAPHIC_DEBUGGING
        // BUG: Doesn't work
        if (key == KEYBOARD_UNSET && get_debugging() == GRAPHIC_DEBUGGING)
            return KEYBOARD_UNSET;
        usleep(10);
    }
    return key;
}

void update_keys(bool *keys) {
    static unsigned long last_pressed;
    unsigned long now = get_time();
    char key = getch();
    if (key == ERR && now - last_pressed < 50000) return;
    for (int i = 0; i < 16; i++) {
        keys[i] = translate(key) == i;
    }
    last_pressed = now;
}

void update_timers(bool *keys) {
    unsigned static long cpu_timers;
    unsigned static long keyboard_timer;
    unsigned long now = get_time();
    if (now - keyboard_timer >= SECOND / 60) {
        update_keys(keys);
        keyboard_timer = now;
    }

    if (now - cpu_timers >= SECOND / 60) {
        Flag timer_flag = decrement_timers();
        st_flash(timer_flag == SOUND);
        cpu_timers = now;
    }
}

void update_io(unsigned int sig, bool *keys) {
    Flag flag = (Flag)(sig & 0xf);
    unsigned char key;

    switch (flag) {
        case DRAW:
        case DRAW_HI_RES:
            draw(get_video_mem(), sig, flag == DRAW_HI_RES);
            break;

        case CLEAR:
            clear_game();
            break;

        case SCROLL:
            redraw_game(get_video_mem(), get_hi_res());
            break;

        case KEYBOARD_BLOCKING:
            key = get_key(keys, KEYBOARD_BLOCKING);
            load_key(KEYBOARD_UNSET, key);
            break;

        case KEYBOARD_NONBLOCKING:
            if (get_debugging() != NO_DEBUGGING) {
                // TODO: Figure out the best way to display this

                // mvprintw(1, 0, "PLEASE PRESS A KEY");
                key = get_key(keys, KEYBOARD_BLOCKING);
                // move(1, 0);
                // clrtoeol();
                // mvprintw(1, 0, "Key pressed: %x", key);
            } else
                key = get_key(keys, KEYBOARD_NONBLOCKING);
            skip_key(KEYBOARD_UNSET, KEYBOARD_UNSET, key);
            break;

        default:
            break;
    }
}

void print_help() {
    printf("Usage: ./chip8_emu [-dcsh] [-t <tick_speed>] <program_path>\n\n");
    printf("Options:\n");
    printf(" -d                Enter graphical debugging mode\n");
    printf(" -c                Enter console debugging mode\n");
    printf(" -s                Enable super-chip8 quirks\n");
    printf(" -t <tick_speed>   Set tick speed (default 900)\n");
    printf(" -h                Displays this message and version number\n");
}

void program_exit() {
    endwin();
    print_error();
    free_assembly();
}

int main(int argc, char *argv[]) {
    int status;
    int tick_speed = DEFAULT_TICK_SPEED;
    bool has_quirks = false;
    signal(SIGTERM, program_exit);

    char option;
    while ((option = getopt(argc, argv, "dcst:h")) != -1) {
        switch (option) {
            case 'd':
                set_debugging(GRAPHIC_DEBUGGING);
                break;
            case 'c':
                set_debugging(CONSOLE_DEBUGGING);
                break;
            case 's':
                has_quirks = true;
                set_superchip8_quirks();
                break;
            case 't':
                tick_speed = atoi(optarg);
                if (tick_speed == 0) tick_speed = DEFAULT_TICK_SPEED;
                break;
            case 'h':
                printf("%s\n", PACKAGE_STRING);
                print_help();
                return 0;
            default:
                print_help();
                return 1;
        }
    }

    if (argc - 1 != optind) {
        // There are more than one non-option arguments
        print_help();
        return 1;
    }

    // Get the first non-option argument
    const char *program_path = argv[optind];
    if (program_path == NULL) {
        print_help();
        return 1;
    }

    // For RND instruction
    srand(time(NULL));

    FILE *program = fopen(program_path, "r");
    if (program == NULL) {
        printf("Error loading %s.\n", program_path);
        return 1;
    }

    init_chip8();
    status = load_program(program);
    if (status == 1) {
        printf("Exiting...\n");
        return 1;
    }

    if (get_debugging() == GRAPHIC_DEBUGGING) {
        set_assembly(program, has_quirks);
    }
    fclose(program);

    while (get_debugging() == CONSOLE_DEBUGGING) {
        // clear screen
        printf("\e[1;1H\e[2J");
        next_cycle();
        printf("\n");
        print_state(get_chip8());
        decrement_timers();
        fgetc(stdin);
    }

    init_graphics(get_debugging() == GRAPHIC_DEBUGGING);
    bool is_key_pressed[16];
    unsigned int flag = IDLE;
    int tick_count = 0;
    while (flag != EXIT) {
        handle_win_size(get_video_mem(), get_hi_res());

        if (get_debugging() == GRAPHIC_DEBUGGING) {
            set_curr_inst(get_chip8()->pc);
            // Run the whole fetch-decode-execute cycle
            for (int i = 0; i < 3; i++) {
                flag = next_cycle();
            }
            tick_count++;
            update_io(flag, is_key_pressed);

            // Update timers per tick
            if (tick_count >= SECOND / 60 / tick_speed) {
                flag = decrement_timers();
                st_flash(flag == SOUND);
                tick_count = 0;
            }

            while (getch() != '\n') {
                handle_win_size(get_video_mem(), get_hi_res());
                usleep(tick_speed);
            }
            continue;
        }

        // Run without debugging
        unsigned long start = get_time();
        handle_xset_message();
        flag = next_cycle();
        update_io(flag, is_key_pressed);
        update_timers(is_key_pressed);
        unsigned long delta = get_time() - start;
        // divide by 3 because fetch-decode and then execute
        if (delta < tick_speed / 3) usleep(tick_speed / 3 - delta);
    }
    program_exit();
    return 0;
}
