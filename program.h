#pragma once

#include "machine.h"
#include <stdint.h> // for uint16_t
#include <stdio.h>  // for FILE *

typedef struct symbol
{
  uint16_t addr, is_set;
  char *label;
  struct symbol *next;
} symbol;

typedef struct instruction
{
  uint16_t addr, inst, flags;
  symbol *sym;
  struct instruction *next, *last;
} instruction;

typedef struct program
{
  uint16_t orig, len;
  instruction *instructions;
  symbol *symbols;
} program;

int disassemble_instruction (char *dest, int flags, symbol *symbols,
                             instruction *inst);

int resolve_symbols (program *prog);
symbol *find_or_create_symbol (program *prog, const char *label);
symbol *find_symbol_by_addr (symbol *symbols, uint16_t addr);
symbol *find_symbol_by_label (symbol *symbols, const char *label);
void sort_symbols_by_addr (program *prog);

void free_instructions (instruction *instructions);
void free_symbols (symbol *symbols);
