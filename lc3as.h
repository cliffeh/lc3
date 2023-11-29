#pragma once

#include "lc3.h"
#include <stdint.h> // for uint16_t

typedef struct instruction
{
  uint16_t addr;
  int op;
  int dr, sr1, sr2;
  int immediate;
  char *label;
} instruction;

typedef struct instruction_list
{
  instruction *head;
  struct inst_list *tail;
} instruction_list;

typedef struct program
{
  uint16_t orig; // starting address
  instruction_list instructions;
} program;
