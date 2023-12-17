#pragma once

#include <stdint.h> // for uint16_t
#include <stdio.h>  // for FILE *

#define MEMORY_MAX (1 << 16)
#define SWAP16(x) ((x << 8) | (x >> 8))
#define SIGN_EXTEND(x, bits)                                                  \
  ((((x) >> ((bits)-1)) & 1) ? ((x) | (0xFFFF << (bits))) : (x))

/* registers */
enum
{
  R_R0 = 0,
  R_R1,
  R_R2,
  R_R3,
  R_R4,
  R_R5,
  R_R6,
  R_R7,
  R_PC, /* program counter */
  R_COND,
  R_COUNT
};

/* op codes */
enum
{
  OP_BR = 0, /* branch */
  OP_ADD,    /* add  */
  OP_LD,     /* load */
  OP_ST,     /* store */
  OP_JSR,    /* jump register */
  OP_AND,    /* bitwise and */
  OP_LDR,    /* load register */
  OP_STR,    /* store register */
  OP_RTI,    /* unused */
  OP_NOT,    /* bitwise not */
  OP_LDI,    /* load indirect */
  OP_STI,    /* store indirect */
  OP_JMP,    /* jump */
  OP_RES,    /* reserved (unused) */
  OP_LEA,    /* load effective address */
  OP_TRAP    /* execute trap */
};

/* condition flags */
enum
{
  FL_POS = 1 << 0, /* P */
  FL_ZRO = 1 << 1, /* Z */
  FL_NEG = 1 << 2, /* N */
};

/* keyboard codes */
enum
{
  MR_KBSR = 0xFE00, /* keyboard status */
  MR_KBDR = 0xFE02  /* keyboard data */
};

/* trap codes */
enum
{
  TRAP_GETC = 0x20, /* get character from keyboard, not echoed */
  TRAP_OUT = 0x21,  /* output a character */
  TRAP_PUTS = 0x22, /* output a word string */
  TRAP_IN = 0x23,   /* get character from keyboard, echoed onto the terminal */
  TRAP_PUTSP = 0x24, /* output a byte string */
  TRAP_HALT = 0x25   /* halt the program */
};

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
  uint16_t flags;
  char *label;
} symbol;

typedef struct program
{
  uint16_t orig, len;
  uint16_t mem[MEMORY_MAX];
  uint16_t reg[R_COUNT];
  symbol *sym[MEMORY_MAX];
  symbol *ref[MEMORY_MAX];
} program;

int assemble_program (program *prog, FILE *in);
int resolve_symbols (program *prog);
int disassemble_addr (char *dest, int flags, uint16_t addr, program *prog);
int print_program (FILE *out, int flags, program *prog);
int load_program (program *prog, FILE *in);
int execute_program (program *prog);
int disassemble_program (program *prog, FILE *symin, FILE *in);
int attach_symbols (program *prog);
int dump_symbols (FILE *out, int flags, program *prog);
int load_symbols (program *prog, FILE *in);

// int disassemble_word (char *dest, int flags, symbol *symbols, uint16_t addr,
//                       uint16_t word);
// int disassemble_instruction (char *dest, int flags, symbol *symbols,
//                              instruction *inst);

//

// int dump_symbols (FILE *out, symbol *symbols);
// symbol *load_symbols (FILE *in);

// int attach_symbols (instruction *instructions, symbol *symbols);

// symbol *find_symbol_by_label (symbol *symbols, const char *label);

// void free_instructions (instruction *instructions);
// void free_symbols (symbol *symbols);
