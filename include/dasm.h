#ifndef DASM_H_
#define DASM_H_

#include <stdbool.h>
#include <stdio.h>

/**
 * Assembly Statement struct
 * @since 1.1.0
 */
typedef struct {
    bool is_directive; /**< Specefies if the statement is a directive (.db for
                          example) */
    char name[5];      /**< The main part of the statement ("JP" for example) */
    char args[4][5];   /**< The five char arguments of the statement */
    unsigned char num_args; /**< The number of arguments of the statement */
    char label[5];          /** The label of a statement **/
} AsmStatement;

/**
 * Dissasemble the programs binary
 * @param program_file: binary's file
 * @param num_statements: pointer to the number of statements in returned binary
 * @return array of assembly statements
 * @since 1.1.0
 */
AsmStatement *disassemble(FILE *program_file, size_t *num_statements);

#endif
