#pragma once

#include "lc3.h"
#include <stdint.h> // for uint16_t
#include <stdio.h>

// print out assembled object code
#define FORMAT_OBJECT 0
// pretty-print the assembly back out
#define FORMAT_ASSEMBLY (1 << 0)
// print instructions as hex
#define FORMAT_HEX (1 << 1)
// print instructions as bit strings
#define FORMAT_BITS (1 << 2)
// include additional debugging information (e.g., instruction addresses)
#define FORMAT_DEBUG (1 << 3)

typedef struct instruction
{
  uint16_t addr, inst;
  int op;
  // TODO these ints could probably be collapsed...
  int reg[3];
  int cond, immediate;
  int imm5, offset6, trapvect8;
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

int generate_code (FILE *out, program *prog, int flags, FILE *debug);
int print_instruction (FILE *out, instruction *inst, int flags);
int char_to_reg (char c);
int find_address_by_label (const instruction_list *instructions,
                           const char *label);
