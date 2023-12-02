#include "lc3as.h"
#include <stdio.h>

// TODO move to util?
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

// TODO move to util?
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

void
print_program (FILE *out, program *prog, int flags)
{
  fprintf (out, ".ORIG x%X\n", prog->orig);
  for (const instruction_list *l = prog->instructions; l; l = l->tail)
    {
      const instruction *inst = l->head;
      // PREAMBLE HERE
      if (flags & FORMAT_DEBUG)
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