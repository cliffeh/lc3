#pragma once

#include "lc3.h"
#include <stdint.h> // for uint16_t

typedef struct instruction
{
  uint16_t addr;
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

void dump_program (program *prog, int format);
void generate_code (program *program);
int char_to_reg (char c);
