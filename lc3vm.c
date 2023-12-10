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

#include <ctype.h> // isprint()
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* unix only */
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/termios.h>
#include <sys/types.h>
#include <unistd.h>

#define MEMORY_MAX (1 << 16)
uint16_t memory[MEMORY_MAX]; /* 65536 locations */
uint16_t reg[R_COUNT];

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
  // TODO handle this for interactive mode (drop back into shell)
  restore_input_buffering ();
  printf ("\n");
  exit (-2);
}

void
read_image_file (FILE *file)
{
  /* the origin tells us where in memory to place the image */
  uint16_t origin;
  size_t read = fread (&origin, sizeof (origin), 1, file);
  origin = SWAP16 (origin);

  /* we know the maximum file size so we only need one fread */
  uint16_t max_read = MEMORY_MAX - origin;
  uint16_t *p = memory + origin;
  read = fread (p, sizeof (uint16_t), max_read, file);

  /* swap to little endian */
  while (read-- > 0)
    {
      *p = SWAP16 (*p);
      ++p;
    }
}

int
read_image (const char *image_path)
{
  FILE *file = fopen (image_path, "rb");
  if (!file)
    {
      return 0;
    };
  read_image_file (file);
  fclose (file);
  return 1;
}

void
load_image (const uint16_t image[], uint16_t len)
{
  if (!len)
    return;

  uint16_t origin = image[0];
  memcpy (memory + origin, image + 1, len - 1);
}

// TODO put this in a header somewhere
int execute_program (uint16_t memory[], uint16_t reg[]);

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

typedef struct command
{
  char *name, *args, *desc;
} command;

static command command_table[]
    = { { "assemble", "[file...]",
          "assemble and load one or more assembly files" },
        { "load", "[file...]", "load one or more object files" },
        { "clear", 0, "clear memory and registers" },
        { "regs", 0, "display the current contents of the registers" },
        { "run", 0, "run the currently-loaded program" },
        { "help", 0, "display this help message" },
        { "exit", 0, "exit the program" },
        0 };

static void
print_help ()
{
  for (command *cmd = command_table; cmd->name; cmd++)
    {
      printf ("%-10s", cmd->name);
      printf ("%-10s", cmd->args ? cmd->args : "");
      printf ("%s\n", cmd->desc);
    }
}

static int
process_command (char *cmd, char *args)
{
  if (strcmp (cmd, "help") == 0)
    {
      print_help ();
    }
  // else if (strcmp (cmd, "assemble") == 0)
  //   {

  //   }
  else if (strcmp (cmd, "load") == 0)
    {
      for (char *p = args; p; p = strtok (0, " ")) // danger!
        {
          printf ("loading %s...", p);

          if (!read_image (p))
            {
              printf ("failed to load image: %s\n", p);
            }
          else
            {
              printf ("successfully loaded\n");
            }
        }
    }
  else if (strcmp (cmd, "run") == 0)
    {
      execute_program (memory, reg);
    }
  else
    {
      printf ("unknown or unimplemented command: %s\n", cmd);
      // print_help ();
    }
}

static void
prompt (char *current_input)
{
  printf ("> ");
  if (current_input)
    printf ("%s", current_input);
}

#define BOOP putc ('\a', stdout)

static int
handle_interactive ()
{
  // TODO define buf size somewhere else
  char buf[1024] = "", c, *cursor = buf, *bufend = buf;
  int running = 1, rc = 0;

  prompt (0);
  do
    {
      switch (c = getchar ())
        {
        case EOF:
          running = 0;
        case 0x04: // ^D
          if (!*buf)
            running = 0;
          break;

        case '\b': // backspace
        case 0x7f: // ^H
          {
            if (cursor > buf) // we're beyond the beginning of the line
              {
                // shift everything left by 1
                for (char *p = cursor - 1; *p; p++)
                  {
                    *p = *(p + 1);
                  }
                putc ('\b', stdout); // back up our cursor

                // hack: append a space to make it "look like" the last
                // character has been removed/everything has been shifted left
                int len = printf ("%s ", --cursor);
                while (len-- > 0)
                  putc ('\b', stdout);
              }
          }
          break;

        case 0x1b: // ESC
          {
            switch (c = getchar ())
              {
              case '[':
                {
                  switch (c = getchar ())
                    {
                      // TODO implement!
                    case 'A': // up arrow
                      break;
                    case 'B': // down arrow
                      break;
                    case 'C': // right arrow
                      {
                        if (*cursor)
                          putc (*cursor++, stdout);
                      }
                      break;
                    case 'D': // left arrow
                      {
                        if (cursor > buf) // cursor is beyond the beginning of
                                          // the line
                          {
                            putc ('\b', stdout);
                            cursor--;
                          }
                      }
                      break;
                    default:
                      {
                        BOOP;
                        fprintf (stderr, "unhandled escape sequence: [%c\n",
                                 c);
                        prompt (buf);
                      }
                    }
                }
                break;

              default:
                {
                  BOOP;
                  fprintf (stderr, "unhandled escape code: %02x\n", c);
                  prompt (buf);
                }
              }
          }
          break;

        case '\t':
          {
            // TODO match on existing input?
            putc ('\n', stdout);
            print_help ();
            prompt (buf);
          }
          break;

        case '\n':
          {
            putc (c, stdout);
            if (*buf) // we have existing input
              {
                // TODO handle this better
                char *cmd = strtok (buf, " ");
                char *args = strtok (0, " ");

                if (cmd)
                  process_command (cmd, args); // TODO capture return

                // clear the buffer
                cursor = buf;
                *cursor = 0;
              }
            prompt (0);
          };
          break;

        default:
          {
            // fprintf (stderr, "\ngot char: %02x\n", c);
            if (isprint (c))
              {
                // if we're appending to the end of the buffer then we want to
                // ensure it remains null-terminated
                if (!*cursor)
                  *(cursor + 1) = 0;

                // if we're not already at the end of the buffer then we need
                // to shift everything to the right 1
                for (char *p = cursor + strlen (cursor); p >= cursor; p--)
                  {
                    *(p + 1) = *p;
                  }

                *cursor = c;
                int len = printf ("%s", cursor++);

                // move the (visible) cursor back to where it should be
                while (--len > 0)
                  putc ('\b', stdout);
              }
            else
              {
                BOOP;
                fprintf (stderr, "unhandled unprintable character: %02x\n", c);
                prompt (buf);
              }
          }
          break;
        }
    }
  while (running);

  return rc;
}

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

  int programs_loaded = 0;
  for (const char *infile = poptGetArg (optCon); infile;
       infile = poptGetArg (optCon))
    {
      if (interactive)
        printf ("loading %s...", infile);

      if (!read_image (infile))
        {
          printf ("failed to load image: %s\n", infile);
          exit (1);
        }
      else
        {
          programs_loaded++;
        }

      if (interactive)
        printf ("successfully loaded\n");
    }

  signal (SIGINT, handle_interrupt);
  disable_input_buffering ();
  if (!interactive)
    {
      rc = execute_program (memory, reg);
    }
  else
    {
      rc = handle_interactive (memory, reg);
    }
  restore_input_buffering ();

cleanup:
  poptFreeContext (optCon);

  exit (rc);
}
