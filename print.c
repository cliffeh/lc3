#include "print.h"

#define SPACES(out, n)                                                        \
  if (n)                                                                      \
  n += fprintf (out, "  ")

int
print_program (FILE *out, int flags, program *prog)
{
  if (!flags)
    return 0;

  int n = 0;

  if (flags & FORMAT_PRETTY)
    fprintf (out, (flags & FORMAT_LC) ? ".orig x%04x" : ".ORIG x%04X\n",
             prog->orig);

  // ensure our symbols are in address order (for pretty-printing below)
  sort_symbols_by_addr (prog);
  symbol *current_symbol = prog->symbols;

  for (instruction *inst = prog->instructions; inst; inst = inst->next)
    {
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
                n += fprintf (out, (flags & FORMAT_LC) ? "%04x" : "%04X",
                              inst->addr + 1);
              SPACES (out, n);
              n += fprintf (out, "%s\n", current_symbol->label);
              current_symbol = current_symbol->next;
            }
        }

      n = 0;

      // NB for the purposes of debugging it's generally more
      // convenient to output addresses relative to .ORIG
      // rather than indexed at zero
      if (flags & FORMAT_ADDR)
        n += fprintf (out, (flags & FORMAT_LC) ? "%04x" : "%04X",
                      inst->addr + 1);

      if (flags & FORMAT_HEX)
        {
          SPACES (out, n);
          uint16_t bytecode = SWAP16 (inst->word);
          n += fprintf (out, (flags & FORMAT_LC) ? "%04x" : "%04X", bytecode);
        }

      if (flags & FORMAT_BITS)
        {
          SPACES (out, n);
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
                SPACES (out, n);
                n += fprintf (out, "  %s", buf);
              }
          }
          break;

        case HINT_FILL:
          {
            if (flags & FORMAT_PRETTY)
              {
                SPACES (out, n);
                n += fprintf (
                    out, (flags & FORMAT_LC) ? "  .fill x%x" : "  .FILL x%X",
                    inst->word);
              }
          }
          break;

        case HINT_STRINGZ:
          {
            if (flags & FORMAT_PRETTY)
              {
                SPACES (out, n);
                n += fprintf (out, (flags & FORMAT_LC) ? "  .stringz \""
                                                       : "  .STRINGZ \"");
                while (inst && inst->word)
                  {
                    char c = (char)inst->word;
                    switch (c)
                      {
                      // clang-format off
                        case '\007': n += fprintf (out, "\\a");  break;
                        case '\013': n += fprintf (out, "\\v");  break;
                        case '\b':   n += fprintf (out, "\\b");  break;
                        case '\e':   n += fprintf (out, "\\e");  break;
                        case '\f':   n += fprintf (out, "\\f");  break;
                        case '\n':   n += fprintf (out, "\\n");  break;
                        case '\r':   n += fprintf (out, "\\r");  break;
                        case '\t':   n += fprintf (out, "\\t");  break;
                        case '\\':   n += fprintf (out, "\\\\"); break;
                        case '"':    n += fprintf (out, "\\\""); break;
                        default:     n += fprintf (out, "%c", c);
                        // clang-format on
                      }

                    inst = inst->next;
                  }

                n += fprintf (out, "\"");
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
      n = 0;
    }

  if (flags & FORMAT_PRETTY)
    fprintf (out, (flags & FORMAT_LC) ? ".end" : ".END\n");

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
