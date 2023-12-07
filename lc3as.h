#pragma once

#include "lc3.h"
#include <stdint.h> // for uint16_t

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

typedef struct symbol
{
  uint16_t addr, is_set;
  char *label;
  struct symbol *next;
} symbol;

typedef struct instruction
{
  uint16_t inst, addr, flags;
  symbol *sym;
  char *pretty;
  struct instruction *next, *last;
} instruction;

typedef struct program
{
  uint16_t orig, len;
  instruction *instructions;
  symbol *symbols;
} program;

symbol *find_or_create_symbol (program *prog, const char *label, uint16_t addr,
                               uint16_t set);
symbol *find_symbol_by_label (symbol *symbols, const char *label);
void sort_symbols_by_addr (program *prog);

void free_instructions(instruction *instructions);
void free_symbols(symbol *symbols);
