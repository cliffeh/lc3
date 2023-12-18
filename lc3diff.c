#define PROGRAM_NAME "lc3diff"
#define PROGRAM_DESCRIPTION "an LC-3 object code diff tool"

#ifdef HAVE_CONFIG_H
#include "config.h"
#define HELP_POSTAMBLE "Report bugs to <" PACKAGE_BUGREPORT ">."
#else
#define PACKAGE_VERSION "unknown"
#endif

#define VERSION_STRING PROGRAM_NAME " " PACKAGE_VERSION

#include "popt/popt.h"
#include "program.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COLOR_RED "\e[31m"
#define COLOR_GRN "\e[32m"
#define COLOR_RST "\e[0m"
#define COLOR_NON ""

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
  poptContext optCon;

  // hack for injecting preamble/postamble into the help message
  struct poptOption emptyTable[] = { POPT_TABLEEND };

  struct poptOption progOptions[]
      = { /* longName, shortName, argInfo, arg, val, descrip, argDescript */
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
  poptSetOtherOptionHelp (optCon, "FILE1 FILE2");

  int rc;
  while ((rc = poptGetNextOpt (optCon)) > 0)
    {
      switch (rc)
        {
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

  FILE *in1, *in2;
  const char *infile1, *infile2;

  if (!(infile1 = poptGetArg (optCon)))
    {
      ERR_EXIT ("two arguments required");
    }
  else if (strcmp (infile1, "-") == 0)
    {
      in1 = stdin;
    }
  else if (!(in1 = fopen (infile1, "r")))
    {
      ERR_EXIT ("couldn't open input file '%s': %s", infile1,
                strerror (errno));
    }

  if (!(infile2 = poptGetArg (optCon)))
    {
      ERR_EXIT ("two arguments required");
    }
  else if (strcmp (infile2, "-") == 0)
    {
      in2 = stdin;
    }
  else if (!(in2 = fopen (infile2, "r")))
    {
      ERR_EXIT ("couldn't open input file '%s': %s", infile2,
                strerror (errno));
    }

  if (in1 == stdin && in2 == stdin)
    ERR_EXIT ("can't compare stdin against itself");

  if (poptGetArg (optCon))
    {
      ERR_EXIT ("unexpected extra argument");
    }

  program prog1, prog2;

  // TODO also load symbols
  if (load_program (&prog1, in1) != 0)
    ERR_EXIT ("unable to load program from %s", infile1);
  if (load_program (&prog1, in2) != 0)
    ERR_EXIT ("unable to load program from %s", infile1);

  uint16_t pos1 = prog1.orig, pos2 = prog2.orig, diff_count = 0;
  if (prog1.orig != prog2.orig)
    {
      fprintf (stderr,
               "warning: programs have different origins: %04x, %04x\n",
               prog1.orig, prog2.orig);
      diff_count++;
    }

  while (pos1 < pos2 && pos1 < prog1.orig + prog1.len)
    {
      printf ("%04x %04x     \n", pos1, prog1.mem[pos1++]);
      diff_count++;
    }

  while (pos2 < pos1 && pos2 < prog2.orig + prog2.len)
    {
      printf ("%04x      %04x\n", pos2, prog2.mem[pos2++]);
      diff_count++;
    }

  // pos1 == pos2

  while (pos1 < prog1.orig + prog1.len && pos2 < prog2.orig + prog2.len)
    { // TODO do the diff
      // if(prog1.mem[pos1] )
    }

  if (diff_count)
    {
      printf ("%d differences\n", diff_count);
      exit (1);
    }

  exit (0);
}