#define PROGRAM_NAME "lc3vm"
#define PROGRAM_DESCRIPTION "an LC-3 virtual machine"

#ifdef HAVE_CONFIG_H
#include "config.h"
#define HELP_POSTAMBLE "Report bugs to <" PACKAGE_BUGREPORT ">."
#else
#define PACKAGE_VERSION "unknown"
#endif

#define VERSION_STRING PROGRAM_NAME " " PACKAGE_VERSION

#include "lc3.h"
#include "popt/popt.h"
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
/* unix only */
#include <fcntl.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/termios.h>
#include <unistd.h>

#define MEMORY_MAX (1 << 16) /* 65536 locations */

struct termios original_tio;

void
disable_input_buffering ()
{
  tcgetattr (STDIN_FILENO, &original_tio);
  struct termios new_tio = original_tio;
  new_tio.c_lflag &= ~ICANON & ~ECHO;
  tcsetattr (STDIN_FILENO, TCSANOW, &new_tio);
}

void
restore_input_buffering ()
{
  tcsetattr (STDIN_FILENO, TCSANOW, &original_tio);
}

void
handle_interrupt (int signal)
{
  restore_input_buffering ();
  printf ("\n");
  exit (-2);
}

int
read_image (uint16_t *memory, FILE *file)
{
  /* the origin tells us where in memory to place the image */
  uint16_t origin;
  size_t read = fread (&origin, sizeof (origin), 1, file);
  origin = SWAP16 (origin);

  /* we know the maximum file size so we only need one fread */
  uint16_t max_read = MEMORY_MAX - origin;
  read += fread (memory, sizeof (uint16_t), max_read, file);

  /* swap to little endian */
  while (read-- > 0)
    {
      *memory = SWAP16 (*memory);
      ++memory;
    }

  // TODO error checking?
  return 0;
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

// TODO put this somewhere else
void execute_program (uint16_t *memory, uint16_t *reg);

int
main (int argc, const char *argv[])
{
  int rc, interactive = 0;

  poptContext optCon;

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
  while ((rc = poptGetNextOpt (optCon)) > 0)
    {
      // TODO parse args
    }

  if (rc != -1)
    {
      ERR_EXIT ("%s: %s\n", poptBadOption (optCon, POPT_BADOPTION_NOALIAS),
                poptStrerror (rc));
    }

  if (!interactive)
    {
      signal (SIGINT, handle_interrupt);
      disable_input_buffering ();
      for (const char *filename = poptGetArg (optCon); filename;
           filename = poptGetArg (optCon))
        {
          FILE *in = fopen (filename, "r");
          if (!in)
            {
              fprintf (stderr, "warning: unable to read from %s\n", filename);
              continue;
            }

          uint16_t *memory = calloc (MEMORY_MAX, sizeof (uint16_t));
          uint16_t *reg = calloc (R_COUNT, sizeof (uint16_t));
          if (read_image (memory, in) != 0)
            {
              printf ("failed to load image: %s\n", filename);
              exit (1);
            }

          execute_program (memory, reg);
        }
      restore_input_buffering ();
    }
  else
    {
      fprintf (stderr, "interactive mode unimplemented!\n");
    }

cleanup:
  poptFreeContext (optCon);
  exit (0); // TODO collect and return exit code(s)
}
