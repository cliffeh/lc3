#define PROGRAM_NAME "lc3as"
#define PROGRAM_DESCRIPTION "an LC-3 assembler"

#ifdef HAVE_CONFIG_H
#include "config.h"
#define HELP_BUGREPORT "Report bugs to <" PACKAGE_BUGREPORT ">."
#else
#define PACKAGE_VERSION "unknown"
#endif

#define VERSION_STRING PROGRAM_NAME " " PACKAGE_VERSION

#include "parse.h"
#include "popt/popt.h"
#include "program.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HELP_PREAMBLE                                                         \
  "If FILE is not provided this program will read from stdin."

#define HELP_POSTAMBLE                                                        \
  "Supported output formats:\n\n"                                             \
  "  o[bject]   output assembled object code (default)\n"                     \
  "  a[ddress]  print the address of each instruction\n"                      \
  "  b[its]     print a binary representation\n"                              \
  "  h[ex]      print a hexadecimal representation\n"                         \
  "  p[retty]   pretty-print the assembly code itself\n"                      \
  "  l[ower]    print everything in lowercase\n"                              \
  "  u[pper]    print everything in uppercase (default)\n"                    \
  "  d[ebug]    shorthand for -Fa -Fh -Fp"

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
  int rc, disassemble = 0, flags = FMT_OBJECT;
  char *outfile = "-", *symbolfile = 0, *format = "object";
  FILE *out = 0, *in = 0, *symfp = 0;

  poptContext optCon;

  // hack for injecting preamble/postamble into the help message
  struct poptOption emptyTable[] = { POPT_TABLEEND };

  struct poptOption progOptions[]
      = { /* longName, shortName, argInfo, arg, val, descrip, argDescript */
          { "disassemble", 'D', POPT_ARG_NONE, &disassemble, 'D',
            "disassemble object code to assembly (implies -Fp)", 0 },
          { "format", 'F', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT,
            &format, 'F', "output format", "FORMAT" },
          { "symbols", 'S', POPT_ARG_STRING, &symbolfile, 'S',
            "also read/write symbols to/from FILE", "FILE" },
          { "output", 'o', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT,
            &outfile, 'o', "write output to FILE", "FILE" },
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
#ifdef HELP_BUGREPORT
    { 0, '\0', POPT_ARG_INCLUDE_TABLE, &emptyTable, 0, HELP_BUGREPORT, 0 },
#endif
    POPT_TABLEEND
  };

  optCon = poptGetContext (0, argc, argv, options, 0);
  poptSetOtherOptionHelp (optCon, "[FILE]");

  while ((rc = poptGetNextOpt (optCon)) > 0)
    {
      switch (rc)
        {
        case 'D':
          flags |= FMT_PRETTY;
          break;

        case 'F':
          {
            if (strcmp (format, "a") == 0 || strcmp (format, "address") == 0)
              {
                flags |= FMT_ADDR;
              }
            else if (strcmp (format, "b") == 0 || strcmp (format, "bits") == 0)
              {
                flags |= FMT_BITS;
              }
            else if (strcmp (format, "d") == 0
                     || strcmp (format, "debug") == 0)
              {
                flags = FMT_DEBUG;
              }
            else if (strcmp (format, "h") == 0 || strcmp (format, "hex") == 0)
              {
                flags |= FMT_HEX;
              }
            else if (strcmp (format, "l") == 0
                     || strcmp (format, "lower") == 0)
              {
                flags |= FMT_LC;
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
                flags |= FMT_PRETTY;
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

        case 'S':
          {
            // nothing we can do here?
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
  else if (!(in = fopen (infile, "r")))
    {
      ERR_EXIT ("couldn't open input file '%s': %s", infile, strerror (errno));
    }

  if (symbolfile)
    {
      if (disassemble)
        { // we're reading symbols
          if (strcmp (symbolfile, "-") == 0)
            {
              if (in == stdin)
                {
                  ERR_EXIT ("stdin specified for both input and symbols");
                }
              symfp = stdin;
            }
          else if (!(symfp = fopen (symbolfile, "r")))
            {
              ERR_EXIT ("couldn't open symbol file '%s': %s", symbolfile,
                        strerror (errno));
            }
        }
      else
        { // we're writing symbols
          if (strcmp (symbolfile, "-") == 0)
            {
              symfp = stderr;
            }
          else if (!(symfp = fopen (symbolfile, "w")))
            {
              ERR_EXIT ("couldn't open symbol file '%s': %s", symbolfile,
                        strerror (errno));
            }
        }
    }
  poptFreeContext (optCon);

  if (!in)
    in = stdin;
  if (!out)
    out = stdout;

  program prog = { .orig = 0, .len = 0, .instructions = 0, .symbols = 0 };

  // TODO implement!
  if (disassemble)
    {
      if ((rc = disassemble_program (&prog, symfp, in)) != 0)
        goto cleanup;

      rc = print_program (out, flags, &prog);
    }
  else
    {
      if ((rc = assemble_program (&prog, in)) != 0)
        goto cleanup;

      rc = print_program (out, flags, &prog);

      if (symfp) // TODO sort?
        dump_symbols (symfp, prog.symbols);
    }

cleanup:
  fclose (in);
  fclose (out);
  if (symfp)
    fclose (symfp);
  free_instructions (prog.instructions);
  free_symbols (prog.symbols);

  exit (rc);
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
