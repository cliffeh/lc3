#include "lc3.h"

#include <stdint.h>
#include <stdio.h>
/* unix only */
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define SIGN_EXTEND(x, bits)                                                  \
  ((((x) >> ((bits)-1)) & 1) ? ((x) | (0xFFFF << (bits))) : (x))

uint16_t
check_key ()
{
  fd_set readfds;
  FD_ZERO (&readfds);
  FD_SET (STDIN_FILENO, &readfds);

  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  return select (1, &readfds, NULL, NULL, &timeout) != 0;
}

void
update_flags (uint16_t reg[], uint16_t r)
{
  if (reg[r] == 0)
    {
      reg[R_COND] = FL_ZRO;
    }
  else if (reg[r] >> 15) /* a 1 in the left-most bit indicates negative */
    {
      reg[R_COND] = FL_NEG;
    }
  else
    {
      reg[R_COND] = FL_POS;
    }
}

void
mem_write (uint16_t memory[], uint16_t address, uint16_t val)
{
  memory[address] = val;
}

uint16_t
mem_read (uint16_t memory[], uint16_t address)
{
  if (address == MR_KBSR)
    {
      if (check_key ())
        {
          memory[MR_KBSR] = (1 << 15);
          memory[MR_KBDR] = getchar ();
        }
      else
        {
          memory[MR_KBSR] = 0;
        }
    }
  return memory[address];
}

int
execute_program (machine *vm)
{
  // for convenience (may refactor later)
  uint16_t *memory = vm->memory;
  uint16_t *reg = vm->reg;

  /* since exactly one condition flag should be set at any given time, set the
   * Z flag */
  reg[R_COND] = FL_ZRO;

  /* set the PC to starting position */
  /* 0x3000 is the default */
  enum
  {
    PC_START = 0x3000
  };
  reg[R_PC] = PC_START;

  int running = 1;
  while (running)
    {
      /* FETCH */
      uint16_t instr = mem_read (memory, reg[R_PC]++);
      uint16_t op = instr >> 12;

      switch (op)
        {
        case OP_ADD:
          {
            /* destination register (DR) */
            uint16_t r0 = (instr >> 9) & 0x7;
            /* first operand (SR1) */
            uint16_t r1 = (instr >> 6) & 0x7;
            /* whether we are in immediate mode */
            uint16_t imm_flag = (instr >> 5) & 0x1;

            if (imm_flag)
              {
                uint16_t imm5 = SIGN_EXTEND (instr & 0x1F, 5);
                reg[r0] = reg[r1] + imm5;
              }
            else
              {
                uint16_t r2 = instr & 0x7;
                reg[r0] = reg[r1] + reg[r2];
              }

            update_flags (reg, r0);
          }
          break;
        case OP_AND:
          {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t r1 = (instr >> 6) & 0x7;
            uint16_t imm_flag = (instr >> 5) & 0x1;

            if (imm_flag)
              {
                uint16_t imm5 = SIGN_EXTEND (instr & 0x1F, 5);
                reg[r0] = reg[r1] & imm5;
              }
            else
              {
                uint16_t r2 = instr & 0x7;
                reg[r0] = reg[r1] & reg[r2];
              }
            update_flags (reg, r0);
          }
          break;
        case OP_NOT:
          {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t r1 = (instr >> 6) & 0x7;

            reg[r0] = ~reg[r1];
            update_flags (reg, r0);
          }
          break;
        case OP_BR:
          {
            uint16_t pc_offset = SIGN_EXTEND (instr & 0x1FF, 9);
            uint16_t cond_flag = (instr >> 9) & 0x7;
            if (cond_flag & reg[R_COND])
              {
                reg[R_PC] += pc_offset;
              }
          }
          break;
        case OP_JMP:
          {
            /* Also handles RET */
            uint16_t r1 = (instr >> 6) & 0x7;
            reg[R_PC] = reg[r1];
          }
          break;
        case OP_JSR:
          {
            uint16_t long_flag = (instr >> 11) & 1;
            reg[R_R7] = reg[R_PC];
            if (long_flag)
              {
                uint16_t long_pc_offset = SIGN_EXTEND (instr & 0x7FF, 11);
                reg[R_PC] += long_pc_offset; /* JSR */
              }
            else
              {
                uint16_t r1 = (instr >> 6) & 0x7;
                reg[R_PC] = reg[r1]; /* JSRR */
              }
          }
          break;
        case OP_LD:
          {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t pc_offset = SIGN_EXTEND (instr & 0x1FF, 9);
            reg[r0] = mem_read (memory, reg[R_PC] + pc_offset);
            update_flags (reg, r0);
          }
          break;
        case OP_LDI:
          {
            /* destination register (DR) */
            uint16_t r0 = (instr >> 9) & 0x7;
            /* PCoffset 9*/
            uint16_t pc_offset = SIGN_EXTEND (instr & 0x1FF, 9);
            /* add pc_offset to the current PC, look at that memory location to
             * get the final address */
            reg[r0]
                = mem_read (memory, mem_read (memory, reg[R_PC] + pc_offset));
            update_flags (reg, r0);
          }
          break;
        case OP_LDR:
          {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t r1 = (instr >> 6) & 0x7;
            uint16_t offset = SIGN_EXTEND (instr & 0x3F, 6);
            reg[r0] = mem_read (memory, reg[r1] + offset);
            update_flags (reg, r0);
          }
          break;
        case OP_LEA:
          {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t pc_offset = SIGN_EXTEND (instr & 0x1FF, 9);
            reg[r0] = reg[R_PC] + pc_offset;
            update_flags (reg, r0);
          }
          break;
        case OP_ST:
          {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t pc_offset = SIGN_EXTEND (instr & 0x1FF, 9);
            mem_write (memory, reg[R_PC] + pc_offset, reg[r0]);
          }
          break;
        case OP_STI:
          {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t pc_offset = SIGN_EXTEND (instr & 0x1FF, 9);
            mem_write (memory, mem_read (memory, reg[R_PC] + pc_offset),
                       reg[r0]);
          }
          break;
        case OP_STR:
          {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t r1 = (instr >> 6) & 0x7;
            uint16_t offset = SIGN_EXTEND (instr & 0x3F, 6);
            mem_write (memory, reg[r1] + offset, reg[r0]);
          }
          break;
        case OP_TRAP:
          reg[R_R7] = reg[R_PC];

          switch (instr & 0xFF)
            {
            case TRAP_GETC:
              /* read a single ASCII char */
              reg[R_R0] = (uint16_t)getchar ();
              update_flags (reg, R_R0);
              break;
            case TRAP_OUT:
              putc ((char)reg[R_R0], stdout);
              fflush (stdout);
              break;
            case TRAP_PUTS:
              {
                /* one char per word */
                uint16_t *c = memory + reg[R_R0];
                while (*c)
                  {
                    putc ((char)*c, stdout);
                    ++c;
                  }
                fflush (stdout);
              }
              break;
            case TRAP_IN:
              {
                printf ("Enter a character: ");
                char c = getchar ();
                putc (c, stdout);
                fflush (stdout);
                reg[R_R0] = (uint16_t)c;
                update_flags (reg, R_R0);
              }
              break;
            case TRAP_PUTSP:
              {
                /* one char per byte (two bytes per word)
                   here we need to swap back to
                   big endian format */
                uint16_t *c = memory + reg[R_R0];
                while (*c)
                  {
                    char char1 = (*c) & 0xFF;
                    putc (char1, stdout);
                    char char2 = (*c) >> 8;
                    if (char2)
                      putc (char2, stdout);
                    ++c;
                  }
                fflush (stdout);
              }
              break;
            case TRAP_HALT:
              // puts("HALT");
              // fflush(stdout);
              running = 0;
              break;
            }
          break;
        case OP_RES:
        case OP_RTI:
        default:
          return -1;
          break;
        }
    }

  return 0;
}
