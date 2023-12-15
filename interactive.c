#include "machine.h"
#include "parse.h"

#include <ctype.h> // isprint()

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
          "file",
          "assemble and load one or more assembly files",
          { "a", 0 } },
        { CMD_LOAD,
          "load",
          "file",
          "load one or more object files",
          { "l", 0 } },
        { CMD_RUN, "run", 0, "run the currently-loaded program", { "r", 0 } },
        { CMD_HELP, "help", 0, "display this help message", { "h", "?", 0 } },
        { CMD_EXIT, "exit", 0, "exit the program", { "quit", "q", "x", 0 } },
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
    printf (STYLE_DIM "%-20s%-12s%s\n" STYLE_RST, "Command", "Arguments",
            "Description");

  for (command *cmd = command_table; cmd->name; cmd++)
    {

      char buf[1024] = "", *aliases = buf, **p = cmd->aliases;
      aliases += sprintf (buf, "%-4s", cmd->name);
      while (*p)
        aliases += sprintf (aliases, ", %s", *p++);

      printf (STYLE_BLD "%-20s" STYLE_RST, buf);
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

    case CMD_EXIT:
      return -1;
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
            else if (assemble_program (&prog, in) != 0)
              {

                printf ("failed to assemble: %s\n", arg);
                error_count++;
              }
            else
              {
                uint16_t orig = prog.orig;
                for (instruction *inst = prog.instructions; inst;
                     inst = inst->next)
                  vm->memory[orig++] = inst->word;

                printf ("successfully loaded\n");
              }

            fclose (in);
            free_instructions (prog.instructions);
            free_symbols (prog.symbols);
          }
      }
      break;

    case CMD_LOAD:
      {
        for (char *arg = args; arg; arg = strtok (0, " ")) // danger!
          {
            printf ("loading %s...", arg);
            FILE *in = fopen (arg, "r");

            if (!in)
              {
                printf ("failed to open: %s\n", arg);
                error_count++;
              }
            else if (load_image (vm->memory, in) != 0)
              {
                printf ("failed to load image: %s\n", arg);
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
      if (execute_machine (vm) != 0)
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

int
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
                  rc = process_command (vm, cmd, args);

                if (rc == -1) // exit
                  running = 0;

                // clear the buffer
                cursor = buf;
                *cursor = 0;
              }

            if (running) // don't re-prompt if we're exiting
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
