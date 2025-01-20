#include <stdlib.h>
#include <string.h>
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

    size_t len;
    AsmStatement *assembly = disassemble(program_file, &len);
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
