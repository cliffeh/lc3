#include "print.h"

int
print_program (FILE *out, int flags, program *prog)
{
  if (!flags)
    return 0;

  if (flags & FORMAT_PRETTY)
    fprintf (out, ".ORIG x%04x\n", prog->orig);

  // ensure our symbols are in address order (for pretty-printing below)
  sort_symbols_by_addr (prog);
  symbol *current_symbol = prog->symbols;

  for (instruction *inst = prog->instructions; inst; inst = inst->next)
    {
      uint16_t bytecode = SWAP16 (inst->inst);

      if (flags & FORMAT_PRETTY)
        {
          // NB if our symbols aren't sorted in address order this
          // won't work the way we expect it to
          while (current_symbol && current_symbol->addr == inst->addr)
            {
              // NB for the purposes of debugging it's generally more
              // convenient to output addresses relative to .ORIG
              // rather than indexed at zero
              if (flags & FORMAT_ADDR)
                fprintf (out, "%04x  ", inst->addr + 1);

              fprintf (out, "%s\n", current_symbol->label);
              current_symbol = current_symbol->next;
            }
        }

      if (inst->pretty) // only print the printable things (not, for
                        // instance, raw string data)
        {
          // NB for the purposes of debugging it's generally more
          // convenient to output addresses relative to .ORIG rather
          // than indexed at zero
          if (flags & FORMAT_ADDR)
            fprintf (out, "%04x", inst->addr + 1);

          if (flags & FORMAT_HEX)
            fprintf (out, "%s%04x", (flags & FORMAT_ADDR) ? "  " : "",
                     bytecode);

          if (flags & FORMAT_BITS)
            {
              if (flags & (FORMAT_ADDR | FORMAT_HEX))
                fprintf (out, "  ");

              for (int i = 15; i >= 0; i--)
                {
                  fprintf (out, "%c", ((inst->inst & (1 << i)) >> i) + '0');
                  if (i && i % 4 == 0)
                    fprintf (out, " ");
                }
            }

          if (flags & FORMAT_PRETTY)
            fprintf (out, "%s  %s",
                     (flags & (FORMAT_ADDR | FORMAT_HEX | FORMAT_BITS)) ? "  "
                                                                        : "",
                     inst->pretty);

          fprintf (out, "\n");
        }
    }

  if (flags & FORMAT_PRETTY)
    fprintf (out, ".END\n");

  return 0;
}