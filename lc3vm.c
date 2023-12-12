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
#include "program.h"
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
read_image_file (uint16_t *memory, FILE *file)
{
  /* the origin tells us where in memory to place the image */
  uint16_t origin;
  size_t read = fread (&origin, sizeof (origin), 1, file);
  origin = SWAP16 (origin);

  /* we know the maximum file size so we only need one fread */
  uint16_t *p = memory + origin;
  read = fread (p, sizeof (uint16_t), (MEMORY_MAX - origin), file);

  /* swap to little endian */
  while (read-- > 0)
    {
      *p = SWAP16 (*p);
      ++p;
    }
}

int // TODO consolidate this?
read_image (uint16_t *memory, const char *image_path)
{
  FILE *file = fopen (image_path, "rb");
  if (!file)
    {
      return 0;
    };
  read_image_file (memory, file);
  fclose (file);
  return 1;
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

#define CMD_UNK 0
#define CMD_ASM 1
#define CMD_LOAD 2
#define CMD_RUN 3
#define CMD_HELP 4
#define CMD_EXIT 99

typedef struct command
{
  int code;
  char *name, *args, *desc, *aliases[8]; // surely 8 is enough for anyone?
} command;

static command command_table[]
    = { /* code, name, args, desc, aliases */
        { CMD_ASM,
          "asm",
          "[file...]",
          "assemble and load one or more assembly files",
          { "a", 0 } },
        { CMD_LOAD,
          "load",
          "[file...]",
          "load one or more object files",
          { "l", 0 } },
        { CMD_RUN, "run", 0, "run the currently-loaded program", { "r", 0 } },
        { CMD_HELP, "help", 0, "display this help message", { "h", "?", 0 } },
        { CMD_EXIT, "quit", 0, "exit the program", { "q", 0 } },
        { CMD_EXIT, "exit", 0, "exit the program", { "x", 0 } },
        { 0, 0, 0, 0, 0 }
      };

#define STYLE_RST "\e[0m"
#define STYLE_BLD "\e[1m"
#define STYLE_DIM "\e[2m"
#define STYLE_ITL "\e[3m"
#define STYLE_UND "\e[4m"

static void
print_help (char *sofar)
{
  if (!sofar)
    printf (STYLE_DIM "%-12s%-12s%s\n" STYLE_RST, "Command", "Arguments",
            "Description");

  for (command *cmd = command_table; cmd->name; cmd++)
    {

      char buf[1024] = "", *aliases = buf, **p = cmd->aliases;
      aliases += sprintf (buf, "%s", cmd->name);
      while (*p)
        aliases += sprintf (aliases, ", %s", *p++);

      printf (STYLE_BLD "%-12s" STYLE_RST, buf);
      printf ("%-12s", cmd->args ? cmd->args : "");
      printf ("%s\n", cmd->desc);
    }
}

static int
parse_command (const char *s)
{
  for (command *cmd = command_table; cmd->name; cmd++)
    {
      if (strcmp (s, cmd->name) == 0)
        return cmd->code;

      for (char **p = cmd->aliases; *p; p++)
        {
          if (strcmp (s, *p) == 0)
            return cmd->code;
        }
    }

  return CMD_UNK;
}

static int
process_command (machine *vm, const char *cmd, char *args)
{
  int error_count = 0;
  switch (parse_command (cmd))
    {
    case CMD_HELP:
      print_help (0);
      break;

    case CMD_ASM:
      {
        for (char *arg = args; arg; arg = strtok (0, " ")) // danger!
          {
            printf ("assembling %s...", arg);

            program prog
                = { .orig = 0, .len = 0, .instructions = 0, .symbols = 0 };
            FILE *in = fopen (arg, "r");

            if (!in)
              {
                printf ("failed to open: %s\n", arg);
                error_count++;
              }
            else if (parse_program (&prog, in) != 0)
              {

                printf ("failed to assemble: %s\n", arg);
                error_count++;
              }
            else if (resolve_symbols (&prog) != 0)
              {
                printf ("symbol resolution error: %s\n", arg);
                error_count++;
              }
            else
              {
                uint16_t orig = prog.orig;
                for (instruction *inst = prog.instructions; inst;
                     inst = inst->next)
                  vm->memory[orig++] = inst->inst;

                printf ("successfully loaded\n");
              }
            free_instructions (prog.instructions);
            free_symbols (prog.symbols);
          }
      }
      break;

    case CMD_LOAD:
      {
        for (char *p = args; p; p = strtok (0, " ")) // danger!
          {
            printf ("loading %s...", p);

            if (!read_image (vm->memory, p))
              {
                printf ("failed to load image: %s\n", p);
                error_count++;
              }
            else
              {
                printf ("successfully loaded\n");
              }
          }
      }
      break;

    case CMD_RUN:
      if (execute_program (vm) != 0)
        error_count++;
      break;

    default:
      printf ("unknown or unimplemented command: %s\n", cmd);
      error_count++;
      break;
    }

  return error_count;
}

#define PROMPT_TEXT "> "

static void
prompt (const char *current_input)
{
  printf (PROMPT_TEXT);
  if (current_input)
    printf ("%s", current_input);
}

static void
extend_cursor (char *cursor, int n)
{
  // ensure we're null-terminated
  int cursor_len = strlen (cursor);
  *(cursor + cursor_len + n) = 0;

  // shift everything to the right n
  for (char *p = cursor + cursor_len; p >= cursor; p--)
    *(p + n) = *p;
}

#define BOOP putc ('\a', stdout)
#define INPUT_BUFFER_SIZE 4096

static int
handle_interactive (machine *vm)
{
  char buf[INPUT_BUFFER_SIZE] = "", cpbuf[INPUT_BUFFER_SIZE] = "", c,
       *cursor = buf;
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

        case 0x01: // ^A (beginning-of-line)
          {
            printf ("\e[3G"); // move cursor to column 3
            cursor = buf;
          }
          break;

        case 0x05: // ^E (end-of-line)
          {
            printf ("%s", cursor);
            cursor = buf + strlen (buf);
          }
          break;

        case 0x0b: // ^K (cut after cursor)
          {
            if (*cursor) // only copy if there's something to copy
              {
                strcpy (cpbuf, cursor);
                *cursor = 0;
                printf ("\e[0K"); // erase to end of line
              }
          }
          break;

        case 0x19: // ^Y (paste buffer)
          {
            // make room for our copy buffer
            int cursorlen = strlen (cursor);
            int pastelen = strlen (cpbuf);
            extend_cursor (cursor, pastelen);

            // paste it and output it
            strncpy (cursor, cpbuf, pastelen);
            printf ("%s", cursor);
            cursor += pastelen;

            // move the (visible) cursor back to where it should be
            while (cursorlen-- > 0)
              putc ('\b', stdout);
          }
          break;

        case 0x0c: // ^L (clear)
          {
            printf ("\e[2J\e[H\e[3J");
            prompt (buf);
          }
          break;

        case '\t':
          {
            // TODO match on existing input?
            putc ('\n', stdout);
            print_help (buf);
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
                  rc = process_command (vm, cmd, args); // TODO capture return

                // clear the buffer
                cursor = buf;
                *cursor = 0;
              }
            prompt (0);
          };
          break;

        default:
          {
            if (isprint (c))
              {
                extend_cursor (cursor, 1);

                *cursor = c;
                int len = printf ("%s", cursor++);

                // move the (visible) cursor back to where it should be
                while (--len > 0)
                  putc ('\b', stdout);
              }
            else
              {
                BOOP;
                fprintf (stderr, "\nunhandled unprintable character: %02x\n",
                         c);
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

  machine vm;
  memset(&vm, 0, sizeof(machine));

  int programs_loaded = 0;
  for (const char *infile = poptGetArg (optCon); infile;
       infile = poptGetArg (optCon))
    {
      if (interactive)
        printf ("loading %s...", infile);

      if (!read_image (vm.memory, infile))
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
      rc = execute_program (&vm);
    }
  else
    {
      rc = handle_interactive (&vm);
    }
  restore_input_buffering ();

cleanup:
  poptFreeContext (optCon);

  exit (rc);
}
