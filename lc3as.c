#ifdef HAVE_CONFIG_H
#include "config.h"
#define VERSION_STRING "lc3as " PACKAGE_VERSION
#endif

#ifndef VERSION_STRING
#define VERSION_STRING "lc3as unknown"
#endif

#include "lc3as.h"
#include "popt/popt.h"
#include "util.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERR_EXIT(args...)                                                     \
  do                                                                          \
    {                                                                         \
      fprintf (stderr, "error: ");                                            \
      fprintf (stderr, args);                                                 \
      fprintf (stderr, "\n");                                                 \
      poptPrintHelp (optCon, stderr, 0);                                      \
      poptFreeContext (optCon);                                               \
      exit (1);                                                               \
    }                                                                         \
  while (0)

extern FILE *yyin;
extern int yyparse (program *prog, int flags, FILE *out);

int
main (int argc, const char *argv[])
{
  int rc, flags = FORMAT_OBJECT;
  char *infile = "-", *outfile = "-", *format = "object";
  FILE *out = 0;
  yyin = 0;

  poptContext optCon;

  struct poptOption options[] = {
    /* longName, shortName, argInfo, arg, val, descrip, argDescript */
    { "format", 'F', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &format, 'F',
      "output format; can be one of a[ddress], b[its], d[ebug], h[ex], "
      "o[bject], p[retty] (note: debug is shorthand for -Fa -Fh -Fp)",
      "FORMAT" },
    { "input", 'i', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &infile, 'i',
      "read input from FILE", "FILE" },
    { "output", 'o', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &outfile,
      'o', "write output to FILE", "FILE" },
    { "version", 0, POPT_ARG_NONE, 0, 'Z', "show version information and exit",
      0 },
    POPT_AUTOHELP POPT_TABLEEND
  };

  optCon = poptGetContext (0, argc, argv, options, 0);

  while ((rc = poptGetNextOpt (optCon)) > 0)
    {
      switch (rc)
        {
        case 'F':
          {
            if (strcmp (format, "a") == 0 || strcmp (format, "address") == 0)
              {
                flags |= FORMAT_ADDR;
              }
            else if (strcmp (format, "b") == 0 || strcmp (format, "bits") == 0)
              {
                flags |= FORMAT_BITS;
              }
            else if (strcmp (format, "d") == 0
                     || strcmp (format, "debug") == 0)
              {
                flags = FORMAT_DEBUG;
              }
            else if (strcmp (format, "h") == 0 || strcmp (format, "hex") == 0)
              {
                flags |= FORMAT_HEX;
              }
            else if (strcmp (format, "o") == 0
                     || strcmp (format, "object") == 0)
              {
                fprintf (stderr, "warning: object format overridden by other "
                                 "format flags\n");
              }
            else if (strcmp (format, "p") == 0
                     || strcmp (format, "pretty") == 0)
              {
                flags |= FORMAT_PRETTY;
              }
            else
              {
                ERR_EXIT ("unknown format specified '%s'", format);
              }
          }
          break;

        case 'i':
          {
            // TODO support >1 input file?
            if (yyin)
              {
                ERR_EXIT ("more than one input file specified");
              }
            else if (strcmp (infile, "-") == 0)
              {
                yyin = stdin;
              }
            else
              {
                if (!(yyin = fopen (infile, "r")))
                  {
                    ERR_EXIT ("couldn't open input file '%s': %s", infile,
                              strerror (errno));
                  }
              }
          }
          break;

        case 'o':
          {
            if (out)
              {
                ERR_EXIT ("more than one output file specified");
              }
            else if (strcmp (outfile, "-") == 0)
              {
                out = stdout;
              }
            else
              {
                if (!(out = fopen (outfile, "w")))
                  {
                    ERR_EXIT ("couldn't open output file '%s': %s", outfile,
                              strerror (errno));
                  }
              }
          }
          break;

        case 'Z':
          {
            printf (VERSION_STRING);
            poptFreeContext (optCon);
            exit (0);
          }
          break;
        }
    }

  if (rc != -1)
    {
      ERR_EXIT ("%s: %s\n", poptBadOption (optCon, POPT_BADOPTION_NOALIAS),
                poptStrerror (rc));
    }

  if (!yyin)
    yyin = stdin;
  if (!out)
    out = stdout;

  program *prog = calloc (1, sizeof (program));

  rc = yyparse (prog, flags, out);

  if (rc == 0)
    {
      rc = resolve_symbols (prog);
      if (rc)
        {
          fprintf (stderr, "%d symbols unresolved; bailing...\n", rc);
          exit (rc);
        }

      if (!flags) // we're supposed to output code
        {
          uint16_t tmp16 = swap16 (prog->orig);
          if (fwrite (&tmp16, sizeof (uint16_t), 1, out) != 1)
            {
              fprintf (stderr, "write error; bailing...\n");
              exit (1);
            }
          for (instruction *inst = prog->instructions; inst; inst = inst->next)
            {
              tmp16 = swap16 (inst->inst);
              if (fwrite (&tmp16, sizeof (uint16_t), 1, out) != 1)
                {
                  fprintf (stderr, "write error; bailing...\n");
                  exit (1);
                }
            }
        }
      // TODO free_program()
    }

  exit (rc);
}

// TODO a more efficient way of looking up label positions
uint16_t
find_position_by_label (const symbol *symbols, const char *label)
{
  for (const symbol *sym = symbols; sym; sym = sym->next)
    {
      if (strcmp (label, sym->label) == 0)
        return sym->pos;
    }
  return 0xFFFF;
}

// TODO move to util?
static const char *
trapvec8_to_str (int trapvec8)
{
  // clang-format off
  switch (trapvec8)
    {
      case 0x20: return "GETC";
      case 0x21: return "OUT";
      case 0x22: return "PUTS";
      case 0x23: return "IN";
      case 0x24: return "PUTSP";
      case 0x25: return "HALT";
    }
  // clang-format on
}

#define PPRINT(out, cp, args...)                                              \
  do                                                                          \
    {                                                                         \
      if (cp)                                                                 \
        cp += fprintf (out, "  ");                                            \
      cp += fprintf (out, args);                                              \
    }                                                                         \
  while (0)

// TODO put this somewhere else?
#define PCOFFSET(curr, dest, bitmask) ((dest - curr - 1) & bitmask)

int
resolve_symbols (program *prog)
{
  int error_count = 0;
  for (instruction *inst = prog->instructions; inst; inst = inst->next)
    {
      if (inst->label) // we have a symbol that needs resolving!
        {
          uint16_t addr = find_position_by_label (prog->symbols, inst->label);
          if (addr == 0xFFFF)
            {
              fprintf (stderr, "error: unresolved symbol: %s\n", inst->label);
              error_count++;
            }
          // TODO we need to check bounds on these somewhere...maybe in the
          // parser?
          if (inst->flags)
            inst->inst |= (((addr - inst->pos) - 1) & inst->flags);
          else
            inst->inst = addr + prog->orig;
        }
    }
  return error_count;
}
