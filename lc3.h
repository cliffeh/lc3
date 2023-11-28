#ifndef LC3_H
#define LC3_H 1

#define MEMORY_MAX (1 << 16)

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
      inst &= 0x0FFF;                                                         \
      inst |= (op << 12);                                                     \
    }                                                                         \
  while (0)

#endif
