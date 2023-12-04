#pragma once

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

#define GET_OP(inst) (inst >> 12)
#define SET_OP(inst, op)                                                      \
  do                                                                          \
    {                                                                         \
      /* zero out the op code just in case */                                 \
      inst &= 0x0FFF;                                                         \
      inst |= (op << 12);                                                     \
    }                                                                         \
  while (0)

// condition flags
enum
{
  FL_POS = 1 << 0, /* P */
  FL_ZRO = 1 << 1, /* Z */
  FL_NEG = 1 << 2, /* N */
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

// 1111 0000 0000 0000
#define MASK_OP 0xF000
// 0000 1110 0000 0000
#define MASK_DR 0x0E00
#define MASK_COND MASK_DR
// 0000 0001 1100 0000
#define MASK_SR1 0x01C0
#define MASK_SR MASK_SR1
#define MASK_BASER MASK_SR1
// 0000 0000 0000 0111
#define MASK_SR2 0x0007
// 0000 0000 0001 1111
#define MASK_IMM5 0x001F
// 0000 0000 0011 1111
#define MASK_OFFSET6 0x003F
// 0000 0001 1111 1111
#define MASK_PCOFFSET9 0x01FF
// 0000 0111 1111 1111
#define MASK_PCOFFSET11 0x7FF
// 0000 0000 0001 0000
#define MASK_BIT5 0x0010
#define MASK_IMM MASK_BIT5
// 0000 1000 0000 0000
#define MASK_BIT11 0x0800

#define GET_DR(inst) ((inst & MASK_DR) >> 9)
#define GET_SR(inst) ((inst & MASK_SR) >> 6)
#define GET_SR1(inst) ((inst & MASK_SR1) >> 6)
#define GET_SR2(inst) ((inst & MASK_SR2))
