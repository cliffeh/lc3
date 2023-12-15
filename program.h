#pragma once

#include "machine.h"
#include <stdint.h> // for uint16_t
#include <stdio.h>  // for FILE *

/* output formatting flags */
#define FMT_OBJECT (0 << 0) // print assembled object code
#define FMT_ADDR (1 << 0)   // include instruction addresses
#define FMT_BITS (1 << 1)   // print instructions as bit strings
#define FMT_HEX (1 << 2)    // print instructions as hex
#define FMT_PRETTY (1 << 3) // pretty-print the assembly back out
#define FMT_LC (1 << 4)     // default: uppercase
#define FMT_DEBUG (FMT_ADDR | FMT_HEX | FMT_PRETTY)

/* disassembler hints */
enum
{
  HINT_INST = 0,
  HINT_FILL,
  HINT_STRINGZ
};

typedef struct symbol
{
  uint16_t addr, is_set, hint;
  char *label;
  struct symbol *next;
} symbol;

typedef struct instruction
{
  uint16_t addr, word, flags, hint;
  symbol *sym;
  struct instruction *next, *last;
} instruction;

typedef struct program
{
  uint16_t orig, len;
  instruction *instructions;
  symbol *symbols;
} program;

int assemble_program (program *prog, FILE *in);
int disassemble_program (program *prog, FILE *symin, FILE *in);

int disassemble_word (char *dest, int flags, symbol *symbols, uint16_t addr,
                      uint16_t word);
int disassemble_instruction (char *dest, int flags, symbol *symbols,
                             instruction *inst);

int print_program (FILE *out, int flags, program *prog);

int dump_symbols (FILE *out, symbol *symbols);
symbol *load_symbols (FILE *in);

int attach_symbols (instruction *instructions, symbol *symbols);
int resolve_symbols (program *prog);
symbol *find_or_create_symbol (program *prog, const char *label);
symbol *find_symbol_by_addr (symbol *symbols, uint16_t addr);
symbol *find_symbol_by_label (symbol *symbols, const char *label);
void sort_symbols_by_addr (program *prog);

void free_instructions (instruction *instructions);
void free_symbols (symbol *symbols);
