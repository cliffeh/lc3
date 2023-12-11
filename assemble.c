#include "lc3as.h"
#include <stdlib.h>
#include <string.h>

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

symbol *
find_symbol_by_label (symbol *symbols, const char *label)
{
  for (symbol *sym = symbols; sym; sym = sym->next)
    {
      if (strcmp (label, sym->label) == 0)
        return sym;
    }
  return 0;
}

symbol *
find_or_create_symbol (program *prog, const char *label)
{
  symbol *sym = find_symbol_by_label (prog->symbols, label);

  if (!sym)
    {
      sym = calloc (1, sizeof (symbol));
      sym->label = strdup (label);
      sym->next = prog->symbols;
      prog->symbols = sym;
    }

  return sym;
}

void
free_instructions (instruction *instructions)
{
  instruction *inst = instructions;

  while (inst)
    {
      if (inst->pretty)
        free (inst->pretty);

      instruction *tmp = inst->next;
      free (inst);
      inst = tmp;
    }
}

void
free_symbols (symbol *symbols)
{
  // TODO figure out why this hangs!
  symbol *sym = symbols;

  while (sym)
    {
      if (sym->label)
        free (sym->label);

      symbol *tmp = sym->next;
      free (sym);
      sym = tmp;
    }
}
