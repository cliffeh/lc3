#include "lc3as.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// print out assembled binary code
#define FORMAT_BIN 0
// pretty-print the assembly back out
#define FORMAT_ASSEMBLY (1 << 0)
// include instruction addresses
#define FORMAT_ASSEMBLY_DEBUG (1 << 1)
// print instructions as hex
#define FORMAT_HEX (1 << 2)
// print instructions as bit strings
#define FORMAT_BITS (1 << 3)

extern int yyparse (program *prog);

static const char *
reg_to_str (int reg)
{
  // clang-format off
  switch(reg)
    {
    case R_R0: return "R0";
    case R_R1: return "R1";
    case R_R2: return "R2";
    case R_R3: return "R3";
    case R_R4: return "R4";
    case R_R5: return "R5";
    case R_R6: return "R6";
    case R_R7: return "R7";
    default:   return "R?";
    }
  // clang-format on
}

static const char *
trapvec8_to_str (int trapvec8)
{
  // clang-format off
  switch (trapvec8)
    {
      case 0x20: return "GETC";
      case 0x21: return "OUT";
      case 0x22: return "PUTS";
      case 0x23: return "IN";
      case 0x24: return "PUTSP";
      case 0x25: return "HALT";
    }
  // clang-format on
}

// TODO a more efficient way of looking up label addresses
static int
find_address_by_label (const instruction_list *instructions, const char *label)
{
  for (const instruction_list *l = instructions; l; l = l->tail)
    {
      const instruction *inst = l->head;
      if (inst->op == -1 && strcmp (label, inst->label) == 0)
        return inst->addr;
    }
  return -1;
}

int
main (int argc, char *argv[])
{
  program *prog = calloc (1, sizeof (program));
  int rc = yyparse (prog), format = FORMAT_ASSEMBLY;

  if (rc == 0)
    {
      dump_program (prog, format);
      // TODO free_program()
      // printf ("addr of label3: %d\n",
      //         find_address_by_label (prog->instructions, "label3"));
    }
  else
    {
      fprintf (stderr, "there was an error\n");
    }

  return rc;
}

void
generate_code (program *prog)
{
  // TODO pass this in?
  FILE *out = stdout;
  for (const instruction_list *l = prog->instructions; l; l = l->tail)
    {
      const instruction *inst = l->head;
      switch (inst->op)
        {
        }
    }
}

void
dump_program (program *prog, int format)
{
  // TODO pass this in?
  FILE *out = stdout;
  fprintf (out, ".ORIG x%X\n", prog->orig);
  for (const instruction_list *l = prog->instructions; l; l = l->tail)
    {
      const instruction *inst = l->head;
      // PREAMBLE HERE
      if (format & FORMAT_ASSEMBLY_DEBUG)
        fprintf (out, "%d  ", inst->addr);
      switch (inst->op)
        {
        case -2:
          {
            fprintf (out, "  .STRINGZ \"%s\"", inst->label);
          }
          break;
        case -1:
          {
            fprintf (out, "%s", inst->label);
          }
          break;
        case OP_ADD:
          {
            if (inst->immediate)
              {
                fprintf (out, "  ADD %s, %s, #%d", reg_to_str (inst->reg[0]),
                         reg_to_str (inst->reg[1]), inst->imm5);
              }
            else
              {
                fprintf (out, "  ADD %s, %s, %s", reg_to_str (inst->reg[0]),
                         reg_to_str (inst->reg[1]), reg_to_str (inst->reg[2]));
              }
          }
          break;
        case OP_AND:
          {
            if (inst->immediate)
              {
                fprintf (out, "  AND %s, %s, #%d", reg_to_str (inst->reg[0]),
                         reg_to_str (inst->reg[1]), inst->imm5);
              }
            else
              {
                fprintf (out, "  AND %s, %s, %s\n", reg_to_str (inst->reg[0]),
                         reg_to_str (inst->reg[1]), reg_to_str (inst->reg[2]));
              }
          }
          break;
        case OP_BR:
          {
            char cond[4] = "", *p = cond;
            if (inst->cond & FL_NEG)
              *p++ = 'n';

            if (inst->cond & FL_ZRO)
              *p++ = 'z';

            if (inst->cond & FL_POS)
              *p++ = 'p';

            fprintf (out, "  BR%s %s", cond, inst->label);
          }
          break;
        case OP_JMP:
          {
            if (inst->immediate)
              fprintf (out, "  JMP %s", reg_to_str (inst->reg[0]));
            else // RET special case
              fprintf (out, "  RET");
          }
          break;
        case OP_JSR:
          {
            if (inst->immediate)
              fprintf (out, "  JSR %s", inst->label);
            else
              fprintf (out, "  JSRR %s", reg_to_str (inst->reg[0]));
          }
          break;
        case OP_LD:
          {
            fprintf (out, "  LD %s, %s", reg_to_str (inst->reg[0]),
                     inst->label);
          }
          break;
        case OP_LDI:
          {
            fprintf (out, "  LDI %s, %s", reg_to_str (inst->reg[0]),
                     inst->label);
          }
          break;
        case OP_LDR:
          {
            fprintf (out, "  LDR %s, %s, #%d", reg_to_str (inst->reg[0]),
                     reg_to_str (inst->reg[1]), inst->offset6);
          }
          break;
        case OP_LEA:
          {
            fprintf (out, "  LEA %s, %s", reg_to_str (inst->reg[0]),
                     inst->label);
          }
          break;
        case OP_NOT:
          {
            fprintf (out, "  NOT %s, %s", reg_to_str (inst->reg[0]),
                     reg_to_str (inst->reg[1]));
          }
          break;
        case OP_RTI:
          {
            fprintf (out, "  RTI");
          }
          break;
        case OP_ST:
          {
            fprintf (out, "  ST %s, %s", reg_to_str (inst->reg[0]),
                     inst->label);
          }
          break;
        case OP_STI:
          {
            fprintf (out, "  STI %s, %s", reg_to_str (inst->reg[0]),
                     inst->label);
          }
          break;
        case OP_STR:
          {
            fprintf (out, "  STR %s, %s, #%d", reg_to_str (inst->reg[0]),
                     reg_to_str (inst->reg[1]), inst->offset6);
          }
          break;
        case OP_TRAP:
          {
            if (inst->immediate)
              fprintf (out, "  TRAP x%x", inst->trapvect8);
            else
              fprintf (out, "  %s", trapvec8_to_str (inst->trapvect8));
          }
          break;
        default:
          {
            fprintf (stderr, "I don't know how to print this op (%d)",
                     inst->op);
          }
        }
      // POSTAMBLE HERE
      fprintf (out, "\n");
    }
  fprintf (out, ".END\n");
}

int
char_to_reg (char c)
{
  // clang-format off
  switch(c)
    {
    case '0': return R_R0;
    case '1': return R_R1;
    case '2': return R_R2;
    case '3': return R_R3;
    case '4': return R_R4;
    case '5': return R_R5;
    case '6': return R_R6;
    case '7': return R_R7;
    default:  return R_COUNT;
    }
  // clang-format on
}
