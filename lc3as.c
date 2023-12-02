#ifdef HAVE_CONFIG_H
#include "config.h"
#define VERSION_STRING "lc3as " PACKAGE_VERSION
#endif

#ifndef VERSION_STRING
#define VERSION_STRING "lc3as unknown"
#endif

#include "lc3as.h"
#include "popt/popt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern FILE *yyin;
extern int yyparse (program *prog);

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
    { "format", 'f', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &format, 'f',
      "output format; can be one of o[bject], a[ssembly], h[ex], b[its]",
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
        case 'f':
          {
          }
          break;

        case 'i':
          {
            // TODO support >1 input file?
            if (yyin)
              {
                fprintf (stderr,
                         "error: more than one input file specified\n");
                poptPrintHelp (optCon, stderr, 0);
                poptFreeContext (optCon);
                exit (1);
              }
            else
              {
                yyin = fopen (infile, "r");
              }
          }
          break;

        case 'o':
          {
            if (out)
              {
                fprintf (stderr,
                         "error: more than one output file specified\n");
                poptPrintHelp (optCon, stderr, 0);
                poptFreeContext (optCon);
                exit (1);
              }
            else
              {
                out = fopen (outfile, "w");
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
      fprintf (stderr, "error: %s: %s\n",
               poptBadOption (optCon, POPT_BADOPTION_NOALIAS),
               poptStrerror (rc));
      poptPrintHelp (optCon, stderr, 0);
      poptFreeContext (optCon);
      exit (1);
    }

  if (!yyin)
    yyin = stdin;
  if (!out)
    out = stdout;

  program *prog = calloc (1, sizeof (program));

  rc = yyparse (prog);

  if (rc == 0)
    {
      print_program (out, prog, flags);
      // TODO free_program()
    }
  else
    {
      fprintf (stderr, "there was an error\n");
    }

  return rc;
}

// TODO a more efficient way of looking up label addresses
int
find_address_by_label (const instruction_list *instructions, const char *label)
{
  for (const instruction_list *l = instructions; l; l = l->tail)
    {
      const instruction *inst = l->head;
      if (inst->op == -1 && strcmp (label, inst->label) == 0)
        return inst->addr;
    }
  return -1;
}

void
generate_code (program *prog)
{
  // TODO pass this in?
  FILE *out = stdout;
  for (const instruction_list *l = prog->instructions; l; l = l->tail)
    {
      const instruction *inst = l->head;
      switch (inst->op)
        {
        }
    }
}



int
char_to_reg (char c)
{
  // clang-format off
  switch(c)
    {
    case '0': return R_R0;
    case '1': return R_R1;
    case '2': return R_R2;
    case '3': return R_R3;
    case '4': return R_R4;
    case '5': return R_R5;
    case '6': return R_R6;
    case '7': return R_R7;
    default:  return R_COUNT;
    }
  // clang-format on
}
