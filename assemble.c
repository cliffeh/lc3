#include "lc3as.h"

int
resolve_symbols (program *prog)
{
  for (instruction *inst = prog->instructions; inst; inst = inst->next)
    {
      if (inst->sym)
        {
          if (!inst->sym->is_set)
            {
              fprintf (stderr, "error: unresolved symbol: %s\n",
                       inst->sym->label);
              return 1;
            }

          if (inst->flags)
            inst->inst |= (((inst->sym->addr - inst->addr) - 1) & inst->flags);
          else
            inst->inst = inst->sym->addr + prog->orig;
        }
    }

  return 0;
}
