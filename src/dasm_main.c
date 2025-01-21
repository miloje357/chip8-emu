#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dasm.h"

void print_statement(AsmStatement stat) {
    if (stat.is_directive) {
        printf("%s: %s %s", stat.label, stat.name, stat.args[0]);
        for (int i = 1; i < stat.num_args; i++) {
            printf(" %s", stat.args[i]);
        }
        printf("\n");
        return;
    }
    if (strlen(stat.label) != 0) {
        printf("\n%s:\n", stat.label);
    }
    printf("\t%s", stat.name);
    for (int i = 0; i < stat.num_args - 1; i++) {
        printf(" %s,", stat.args[i]);
    }
    if (stat.num_args != 0) {
        printf(" %s", stat.args[stat.num_args - 1]);
    }
    printf("\n");
}

void print_help() {
    printf("Usage: ./chip8_dasm [-sh] <program_path>\n");
    printf("Options:\n");
    printf(" -s                Enable super-chip8 quirks\n");
    printf(" -h                Displays this message and version number\n");
}

int main(int argc, char *argv[]) {
    bool has_superchip8_quirks = false;

    char arg;
    while ((arg = getopt(argc, argv, "sh")) != -1) {
        switch (arg) {
            case 's':
                has_superchip8_quirks = true;
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
    FILE *program_file = fopen(argv[optind], "r");
    if (program_file == NULL) {
        printf("Error loading %s.\n", argv[optind]);
        printf("Exiting...\n");
        return 1;
    }

    size_t len;
    AsmStatement *assembly =
        disassemble(program_file, &len, has_superchip8_quirks);
    if (assembly == NULL) {
        printf("Couldn't dissasemble program. Exiting...\n");
        fclose(program_file);
        return 1;
    }
    fclose(program_file);

    for (int i = 0; i < len; i++) {
        print_statement(assembly[i]);
    }
    free(assembly);
    return 0;
}
