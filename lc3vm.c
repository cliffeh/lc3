#define PROGRAM_NAME "lc3vm"
#define PROGRAM_DESCRIPTION "an LC-3 virtual machine"

#ifdef HAVE_CONFIG_H
#include "config.h"
#define HELP_POSTAMBLE "Report bugs to <" PACKAGE_BUGREPORT ">."
#else
#define PACKAGE_VERSION "unknown"
#endif

#define VERSION_STRING PROGRAM_NAME " " PACKAGE_VERSION

#include "parse.h"
#include "popt/popt.h"
#include "program.h"

#include <signal.h> // signal()
#include <stdint.h> // uint16_t
#include <stdio.h>
/* unix only */
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/termios.h>
#include <sys/types.h>
#include <unistd.h>

struct termios original_tio;

static void
disable_input_buffering ()
{
  tcgetattr (STDIN_FILENO, &original_tio);
  struct termios new_tio = original_tio;
  new_tio.c_lflag &= ~ICANON & ~ECHO;
  tcsetattr (STDIN_FILENO, TCSANOW, &new_tio);
}

static void
restore_input_buffering ()
{
  tcsetattr (STDIN_FILENO, TCSANOW, &original_tio);
}

static void
handle_interrupt (int signal)
{
  // TODO handle this for interactive mode (drop back into shell)
  restore_input_buffering ();
  printf ("\n");
  exit (-2);
}

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

// TODO put this in a header somewhere?
int handle_interactive (machine *vm);

int
main (int argc, const char *argv[])
{
  poptContext optCon;
  int interactive = 0;

  // hack for injecting preamble/postamble into the help message
  struct poptOption emptyTable[] = { POPT_TABLEEND };

  struct poptOption progOptions[]
      = { /* longName, shortName, argInfo, arg, val, descrip, argDescript */
          { "interactive", 'i', POPT_ARG_NONE, &interactive, 'i',
            "run in interactive mode", 0 },
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
  poptSetOtherOptionHelp (optCon, "[FILE...]");

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

  machine vm;
  memset (&vm, 0, sizeof (machine));

  int programs_loaded = 0;
  for (const char *infile = poptGetArg (optCon); infile;
       infile = poptGetArg (optCon))
    {
      if (interactive)
        printf ("loading %s...", infile);

      FILE *in = fopen (infile, "r");
      if (!in)
        {
          fprintf (stderr, "error: failed to open %s: %s", infile,
                   strerror (errno));
          exit (1);
        }

      if (load_image (vm.memory, in) != 0)
        {
          fprintf (stderr, "failed to load image: %s\n", infile);
          exit (1);
        }
      fclose (in);

      programs_loaded++;

      if (interactive)
        printf ("successfully loaded\n");
    }
  poptFreeContext (optCon);

  signal (SIGINT, handle_interrupt);
  disable_input_buffering ();
  if (!interactive)
    {
      rc = execute_machine (&vm);
    }
  else
    {
      rc = handle_interactive (&vm);
    }
  restore_input_buffering ();

  exit (rc);
}
