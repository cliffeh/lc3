#include "program.h"

#define SPACES(out, n)                                                        \
  if (n)                                                                      \
  n += fprintf (out, "  ")

int
print_program (FILE *out, int flags, program *prog)
{
  if (!flags) // write  assembled object code
    {
      uint16_t bytecode = SWAP16 (prog->orig);
      if (fwrite (&bytecode, sizeof (uint16_t), 1, out) != 1)
        {
          fprintf (stderr, "write error...bailing.\n");
          return 1;
        }
      for (int i = prog->orig; i < prog->orig + prog->len; i++)
        {
          bytecode = SWAP16 (prog->mem[i]);
          if (fwrite (&bytecode, sizeof (uint16_t), 1, out) != 1)
            {
              fprintf (stderr, "write error...bailing.\n");
              return 1;
            }
        }

      return 0;
    }

  if (flags & FMT_PRETTY)
    fprintf (out, (flags & FMT_LC) ? ".orig x%04x" : ".ORIG x%04X\n",
             prog->orig);

  for (int i = prog->orig; i < prog->orig + prog->len; i++)
    {
      if (flags & FMT_PRETTY && prog->sym[i] && *prog->sym[i]->label != '_')
        {
          int n = 0;
          if (flags & FMT_ADDR)
            n += fprintf (out, (flags & FMT_LC) ? "%04x" : "%04X", i);
          SPACES (out, n);
          n += fprintf (out, "%s\n", prog->sym[i]->label);
        }

      int n = 0;

      if (flags & FMT_ADDR)
        n += fprintf (out, (flags & FMT_LC) ? "%04x" : "%04X", i);

      if (flags & FMT_HEX)
        {
          SPACES (out, n);
          uint16_t bytecode = SWAP16 (prog->mem[i]);
          n += fprintf (out, (flags & FMT_LC) ? "%04x" : "%04X", bytecode);
        }

      if (flags & FMT_BITS)
        {
          SPACES (out, n);
          for (int i = 15; i >= 0; i--)
            {
              n += fprintf (out, "%c", ((prog->mem[i] & (1 << i)) >> i) + '0');
              if (i && i % 4 == 0)
                n += fprintf (out, " ");
            }
        }

      char buf[4096] = "";
      i += disassemble_addr (buf, flags, i, prog);
      if (flags & FMT_PRETTY)
        fprintf (out, "  %s\n", buf);
    }

  if (flags & FMT_PRETTY)
    fprintf (out, (flags & FMT_LC) ? ".end" : ".END\n");

  return 0;
}

int
dump_symbols (FILE *out, int flags, program *prog)
{
  for (int saddr = prog->orig; saddr < prog->orig + prog->len; saddr++)
    {
      if (prog->sym[saddr])
        {
          fprintf (out, (flags & FMT_LC) ? "x%04x %s" : "x%0X %s", saddr,
                   prog->sym[saddr]->label);
          if (prog->sym[saddr]->flags >> 12)
            fprintf (out, " %d", (prog->sym[saddr]->flags >> 12));
          fprintf (out, "\n");
        }
    }
}
