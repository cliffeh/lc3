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
  /*for(instruction_list *l = prog->instructions; l; l = l->tail){
    instruction *inst = l->head;
    fprintf (out, "%s\n", inst->line_label);
  }*/
}
