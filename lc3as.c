#define PROGRAM_NAME "lc3as"
#define PROGRAM_DESCRIPTION "an LC-3 assembler"

#ifdef HAVE_CONFIG_H
#include "config.h"
#define HELP_POSTAMBLE "Report bugs to <" PACKAGE_BUGREPORT ">."
#else
#define PACKAGE_VERSION "unknown"
#endif

#define VERSION_STRING PROGRAM_NAME " " PACKAGE_VERSION

#include "lc3as.h"
#include "parse.h"
#include "popt/popt.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HELP_PREAMBLE                                                         \
  "Note: if FILE is not provided this program will read from stdin."

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

int
main (int argc, const char *argv[])
{
  int rc, flags = FORMAT_OBJECT;
  char *outfile = "-", *format = "object";
  FILE *out = 0, *in = 0;

  poptContext optCon;

  // hack for injecting preamble/postamble into the help message
  struct poptOption emptyTable[] = { POPT_TABLEEND };

  struct poptOption progOptions[] = {
    /* longName, shortName, argInfo, arg, val, descrip, argDescript */
    { "format", 'F', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &format, 'F',
      "output format; can be one of a[ddress], b[its], d[ebug], h[ex], "
      "o[bject], p[retty] (note: debug is shorthand for -Fa -Fh -Fp)",
      "FORMAT" },
    { "output", 'o', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &outfile,
      'o', "write output to FILE", "FILE" },
    { "version", '\0', POPT_ARG_NONE, 0, 'V',
      "show version information and exit", 0 },
    POPT_TABLEEND
  };

  struct poptOption options[] = {
#ifdef HELP_PREAMBLE
    { 0, '\0', POPT_ARG_INCLUDE_TABLE, &emptyTable, 0, HELP_PREAMBLE, 0 },
#endif
    { 0, '\0', POPT_ARG_INCLUDE_TABLE, &progOptions, 0, "Options:", 0 },
    POPT_AUTOHELP
#ifdef HELP_POSTAMBLE
    { 0, '\0', POPT_ARG_INCLUDE_TABLE, &emptyTable, 0, HELP_POSTAMBLE, 0 },
#endif
    POPT_TABLEEND
  };

  optCon = poptGetContext (0, argc, argv, options, 0);
  poptSetOtherOptionHelp (optCon, "[FILE]");

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
            free (format);
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
            free (outfile);
          }
          break;

        case 'V':
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

  const char *infile = poptGetArg (optCon);

  if (poptGetArg (optCon))
    {
      // TODO maybe support more than one input file
      ERR_EXIT ("more than one input file specified");
    }
  else if (!infile || strcmp (infile, "-") == 0)
    {
      in = stdin;
    }
  else
    {
      if (!(in = fopen (infile, "r")))
        {
          ERR_EXIT ("couldn't open input file '%s': %s", infile,
                    strerror (errno));
        }
    }

  if (!in)
    in = stdin;
  if (!out)
    out = stdout;

  program prog = { .orig = 0, .len = 0, .instructions = 0, .symbols = 0 };

  if (flags)
    {
      if ((rc = parse_program (&prog, in)) == 0)
        {
          rc = print_program (out, flags, &prog);
        }
    }
  else
    {
      // TODO use MEMORY_MAX?
      uint16_t bytecode[1 << 16];
      int len = assemble_program (bytecode, in);
      rc = (fwrite (bytecode, sizeof (uint16_t), len, out) == len) ? 0 : 1;
    }

cleanup:
  fclose (in);
  fclose (out);
  free_instructions (prog.instructions);
  free_symbols (prog.symbols);
  poptFreeContext (optCon);

  exit (rc);
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

static int
compare_symbol_addrs (const void *sym1, const void *sym2)
{
  int addr1 = (*(symbol **)sym1)->addr;
  int addr2 = (*(symbol **)sym2)->addr;

  return addr1 - addr2;
}

void // TODO there is probably a cleaner way to do this
sort_symbols_by_addr (program *prog)
{
  if (!prog->symbols || !prog->symbols->next)
    return;

  // figure out how many symbols we have
  int len = 0;
  for (symbol *sym = prog->symbols; sym; sym = sym->next)
    len++;

  // ...pack them all into an array so we can sort it using qsort()
  symbol **symbols = malloc (len * sizeof (symbol *) + sizeof (symbol **));
  len = 0;
  for (symbol *sym = prog->symbols; sym; sym = sym->next)
    symbols[len++] = sym;

  // ...sort it
  qsort (symbols, len, sizeof (symbol *), compare_symbol_addrs);

  // ...and then pack them all back into a linked list
  symbol *sym = prog->symbols = symbols[0];
  for (int i = 1; i < len; i++)
    {
      sym->next = symbols[i];
      sym = sym->next;
    }
  sym->next = 0;

  free (symbols);
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
