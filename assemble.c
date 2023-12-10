#include "lc3as.h"

int
resolve_symbols (program *prog, instruction *inst)
{
  if (inst->sym)
    {
      if (!inst->sym->is_set)
        {
          fprintf (stderr, "error: unresolved symbol: %s\n", inst->sym->label);
          return 1;
        }

      if (inst->flags)
        inst->inst |= (((inst->sym->addr - inst->addr) - 1) & inst->flags);
      else
        inst->inst = inst->sym->addr + prog->orig;
    }

  return 0;
}

int
assemble_program (uint16_t dest[], FILE *in)
{
  program prog = { .orig = 0, .len = 0, .instructions = 0, .symbols = 0 };

  parse_program (&prog, in);

  int i = 0;
  dest[i++] = prog.orig;
  for (instruction *inst = prog.instructions; inst; inst = inst->next)
    {
      resolve_symbols (&prog, inst);
      dest[i++] = inst->inst;
    }

  return i;
}