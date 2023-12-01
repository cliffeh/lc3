#include "lc3as.h"
#include <stdio.h>
#include <stdlib.h>

extern int yyparse(program *prog);

int
main (int argc, char *argv[])
{
  program *prog = calloc (1, sizeof (program));
  int rc = yyparse (prog);

  if (rc == 0)
    {
      dump_program (prog);
      // TODO free_program()
    }
  else
    {
      fprintf (stderr, "there was an error\n");
    }

  return rc;
}

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

void
dump_program (program *prog)
{
  // TODO pass this in?
  FILE *out = stdout;
  fprintf (out, ".ORIG x%X\n", prog->orig);
  for (instruction_list *l = prog->instructions; l; l = l->tail)
    {
      instruction *inst = l->head;
      // uncomment to debug addresses!
      // fprintf (out, "%d  ", inst->addr);
      switch (inst->op)
        {
        case -1:
          {
            fprintf (out, "%s\n", inst->label);
          }
          break;
        case OP_ADD:
          {
            if (inst->immediate)
              {
                fprintf (out, "  ADD %s, %s, #%d\n", reg_to_str (inst->reg[0]),
                         reg_to_str (inst->reg[1]), inst->imm5);
              }
            else
              {
                fprintf (out, "  ADD %s, %s, %s\n", reg_to_str (inst->reg[0]),
                         reg_to_str (inst->reg[1]), reg_to_str (inst->reg[2]));
              }
          }
          break;
        case OP_AND:
          {
            if (inst->immediate)
              {
                fprintf (out, "  AND %s, %s, #%d\n", reg_to_str (inst->reg[0]),
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

            fprintf (out, "  BR%s %s\n", cond, inst->label);
          }
          break;
        case OP_JMP:
          {
            if (inst->immediate) // RET special case
              fprintf (out, "  RET\n");
            else
              fprintf (out, "  JMP %s\n", reg_to_str (inst->reg[0]));
          }
          break;
        case OP_JSR:
          {
            if (inst->immediate)
              fprintf (out, "  JSR %s\n", inst->label);
            else
              fprintf (out, "  JSRR %s\n", reg_to_str (inst->reg[0]));
          }
          break;
        case OP_LD:
          {
            fprintf (out, "  LD %s, %s\n", reg_to_str (inst->reg[0]),
                     inst->label);
          }
          break;
        case OP_LDI:
          {
            fprintf (out, "  LDI %s, %s\n", reg_to_str (inst->reg[0]),
                     inst->label);
          }
          break;
        case OP_LDR:
          {
            fprintf (out, "  LDR %s, %s, #%d\n", reg_to_str (inst->reg[0]),
                     reg_to_str (inst->reg[1]), inst->offset6);
          }
          break;
        case OP_LEA:
          {
            fprintf (out, "  LEA %s, %s\n", reg_to_str (inst->reg[0]),
                     inst->label);
          }
          break;
        case OP_NOT:
          {
            fprintf (out, "  NOT %s, %s\n", reg_to_str (inst->reg[0]),
                     reg_to_str (inst->reg[1]));
          }
          break;
        case OP_RTI:
          {
            fprintf (out, "  RTI\n");
          }
          break;
        case OP_ST:
          {
            fprintf (out, "  ST %s, %s\n", reg_to_str (inst->reg[0]),
                     inst->label);
          }
          break;
        case OP_STI:
          {
            fprintf (out, "  STI %s, %s\n", reg_to_str (inst->reg[0]),
                     inst->label);
          }
          break;
        case OP_STR:
          {
            fprintf (out, "  STR %s, %s, #%d\n", reg_to_str (inst->reg[0]),
                     reg_to_str (inst->reg[1]), inst->offset6);
          }
          break;
        case OP_TRAP:
          {
            fprintf (out, "  TRAP x%x\n", inst->trapvect8);
          }
          break;
        default:
          {
            fprintf (stderr, "I don't know how to print this op\n");
          }
        }
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
