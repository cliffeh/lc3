#pragma once

#include "lc3.h"
#include <stdint.h> // for uint16_t
#include <stdio.h>

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
// debugging output
#define FORMAT_DEBUG (FORMAT_ADDR|FORMAT_BITS|FORMAT_HEX|FORMAT_PRETTY)

typedef struct instruction
{
  uint16_t inst;
  int op;
  int pos;
  // TODO these ints could probably be collapsed...
  int reg[3];
  int cond, immediate;
  int imm5, offset6, trapvect8;
  uint16_t fill_value;
  char *label;
} instruction;

typedef struct instruction_list
{
  instruction *head;
  struct instruction_list *tail;
  struct instruction_list *last;
} instruction_list;

typedef struct program
{
  uint16_t orig; // starting address
  uint16_t len;  // total length of program
  instruction_list *instructions;
} program;

int generate_code (FILE *out, program *prog, int flags);
int print_instruction (FILE *out, instruction *inst, int flags);
int char_to_reg (char c);
int find_position_by_label (const instruction_list *instructions,
                           const char *label);
