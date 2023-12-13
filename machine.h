#pragma once

#include <stdint.h> // uint16_t
#include <stdio.h>

#define MEMORY_MAX (1 << 16)
#define SWAP16(x) ((x << 8) | (x >> 8))
#define SIGN_EXTEND(x, bits)                                                  \
  ((((x) >> ((bits)-1)) & 1) ? ((x) | (0xFFFF << (bits))) : (x))

// registers
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

// op codes
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

// condition flags
enum
{
  FL_POS = 1 << 0, /* P */
  FL_ZRO = 1 << 1, /* Z */
  FL_NEG = 1 << 2, /* N */
};

// keyboard codes
enum
{
  MR_KBSR = 0xFE00, /* keyboard status */
  MR_KBDR = 0xFE02  /* keyboard data */
};

// trap codes
enum
{
  TRAP_GETC = 0x20, /* get character from keyboard, not echoed */
  TRAP_OUT = 0x21,  /* output a character */
  TRAP_PUTS = 0x22, /* output a word string */
  TRAP_IN = 0x23,   /* get character from keyboard, echoed onto the terminal */
  TRAP_PUTSP = 0x24, /* output a byte string */
  TRAP_HALT = 0x25   /* halt the program */
};

typedef struct machine
{
  uint16_t memory[MEMORY_MAX];
  uint16_t reg[R_COUNT];
} machine;

int execute_machine (machine *vm);        // execute.c
int load_machine (machine *vm, FILE *in); // machine.c
