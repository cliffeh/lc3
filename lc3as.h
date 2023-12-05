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
#define FORMAT_DEBUG (FORMAT_ADDR | FORMAT_HEX | FORMAT_PRETTY)

typedef struct instruction
{
  uint16_t inst, addr, flags;
  char *label, *pretty;
  struct instruction *next, *last;
} instruction;

typedef struct symbol
{
  uint16_t addr;
  char *label;
  struct symbol *next;
} symbol;

typedef struct program
{
  uint16_t orig, len;
  instruction *instructions;
  symbol *symbols;
} program;

int resolve_symbols (program *prog);
uint16_t find_addr_by_label (const symbol *symbols, const char *label);
