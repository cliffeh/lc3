#pragma once

#include "lc3.h"
#include <stdint.h> // for uint16_t

typedef struct instruction
{
  uint16_t addr;
  int op;
  // TODO these ints could probably be collapsed...
  int dr, sr1, sr2;
  int immediate, imm5, offset6, cond;
  char *label;
} instruction;

typedef struct instruction_list
{
  instruction *head;
  struct instruction_list *tail;
} instruction_list;

typedef struct program
{
  uint16_t orig; // starting address
  instruction_list *instructions;
} program;

void dump_program(program *prog);
int char_to_reg(char c);
