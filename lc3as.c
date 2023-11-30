#include "lc3as.h"
#include <stdio.h>

extern int yyparse (program **prog);

int
main (int argc, char *argv[])
{
  program *prog;
  int rc;

  rc = yyparse (&prog);

  if (rc == 0)
    {
      dump_program (prog);
    }
  else
    {
      fprintf (stderr, "there was an error\n");
    }

  return rc;
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
                fprintf (out, "ADD R%d, R%d, #%d\n", inst->dr, inst->sr1,
                         inst->imm5);
              }
            else
              {
                fprintf (out, "ADD R%d, R%d, R%d\n", inst->dr, inst->sr1,
                         inst->sr2);
              }
          }
          break;
        case OP_AND:
          {
            if (inst->immediate)
              {
                fprintf (out, "AND R%d, R%d, #%d\n", inst->dr, inst->sr1,
                         inst->imm5);
              }
            else
              {
                fprintf (out, "AND R%d, R%d, R%d\n", inst->dr, inst->sr1,
                         inst->sr2);
              }
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
