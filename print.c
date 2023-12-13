#include "print.h"

int
print_program (FILE *out, int flags, program *prog)
{
  if (!flags)
    return 0;

  int n = 0;

  if (flags & FORMAT_PRETTY)
    fprintf (out, ".ORIG x%04x\n", prog->orig);

  // ensure our symbols are in address order (for pretty-printing below)
  sort_symbols_by_addr (prog);
  symbol *current_symbol = prog->symbols;

  for (instruction *inst = prog->instructions; inst; inst = inst->next)
    {
      // NB for the purposes of debugging it's generally more
      // convenient to output addresses relative to .ORIG
      // rather than indexed at zero
      if (flags & FORMAT_ADDR)
        n += fprintf (out, "%04x", inst->addr + 1);

      if (flags & FORMAT_PRETTY)
        {
          // NB if our symbols aren't sorted in address order this
          // won't work the way we expect it to
          while (current_symbol && current_symbol->addr == inst->addr)
            {
              n += fprintf (out, "%s%s\n", n ? "  " : "",
                            current_symbol->label);
              current_symbol = current_symbol->next;
            }
        }

      if (flags & FORMAT_HEX)
        {
          n += fprintf (out, "%s%04x", n ? "  " : "", SWAP16 (inst->word));
        }

      if (flags & FORMAT_BITS)
        {
          n += fprintf (out, "%s", n ? "  " : "");

          for (int i = 15; i >= 0; i--)
            {
              n += fprintf (out, "%c", ((inst->word & (1 << i)) >> i) + '0');
              if (i && i % 4 == 0)
                n += fprintf (out, " ");
            }
        }

      char buf[4096];
      switch (inst->hint)
        {
        case HINT_INST:
          {
            if (flags & FORMAT_PRETTY)
              {
                disassemble_instruction (buf, flags, prog->symbols, inst);
                n += fprintf (out, "%s  %s", n ? "  " : "", buf);
              }
          }
          break;

        case HINT_FILL:
          {
            if (flags & FORMAT_PRETTY)
              n += fprintf (out, "%s.FILL x%x", n ? "  " : "", inst->word);
          }
          break;

        case HINT_STRINGZ:
          {
            if (flags & FORMAT_PRETTY)
              {
                n += fprintf (out, "%s.STRINGZ", n ? "  " : "");
                while (inst && inst->word)
                  {
                    // TODO UNESCAPE!
                    n += fprintf (out, "%c", ((char)inst->word));
                    inst = inst->next;
                  }
              }
            else
              {
                // still need to skip past the characters...
                while (inst && inst->word)
                  {
                    inst = inst->next;
                  }
              }
          }
          break;
        }

      fprintf (out, "\n");
    }

  if (flags & FORMAT_PRETTY)
    fprintf (out, ".END\n");

  return 0;
}

int
write_bytecode (FILE *out, program *prog)
{
  uint16_t bytecode = SWAP16 (prog->orig);
  if (fwrite (&bytecode, sizeof (uint16_t), 1, out) != 1)
    {
      fprintf (stderr, "write error...bailing.\n");
      return 1;
    }
  for (instruction *inst = prog->instructions; inst; inst = inst->next)
    {
      bytecode = SWAP16 (inst->word);
      if (fwrite (&bytecode, sizeof (uint16_t), 1, out) != 1)
        {
          fprintf (stderr, "write error...bailing.\n");
          return 1;
        }
    }

  return 0;
}