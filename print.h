#pragma once

#include "parse.h"
#include "program.h"
#include <stdio.h> // FILE *

// print out assembled object code
#define FORMAT_OBJECT 0
// include instruction addresses
#define FORMAT_ADDR (1 << 0)
// print instructions as bit strings
#define FORMAT_BITS (1 << 1)
// print instructions as hex
#define FORMAT_HEX (1 << 2)
// pretty-print the assembly back out
#define FORMAT_PRETTY (1 << 3)
// default: uppercase
#define FORMAT_LC (1 << 4)
// debugging output
#define FORMAT_DEBUG (FORMAT_ADDR | FORMAT_HEX | FORMAT_PRETTY)

int print_program (FILE *out, int flags, program *prog);
int write_bytecode (FILE *out, program *prog);
