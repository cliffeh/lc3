#define PROGRAM_NAME "lc3vm"
#define PROGRAM_DESCRIPTION "an LC-3 virtual machine"

#ifdef HAVE_CONFIG_H
#include "config.h"
#define HELP_POSTAMBLE "Report bugs to <" PACKAGE_BUGREPORT ">."
#else
#define PACKAGE_VERSION "unknown"
#endif

#define VERSION_STRING PROGRAM_NAME " " PACKAGE_VERSION

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
/* unix only */
#include "lc3.h"
#include "popt/popt.h"
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/termios.h>
#include <sys/time.h>
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

uint16_t
check_key ()
{
  fd_set readfds;
  FD_ZERO (&readfds);
  FD_SET (STDIN_FILENO, &readfds);

  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  return select (1, &readfds, NULL, NULL, &timeout) != 0;
}

void
handle_interrupt (int signal)
{
  restore_input_buffering ();
  printf ("\n");
  exit (-2);
}

void
update_flags (uint16_t r)
{
  if (reg[r] == 0)
    {
      reg[R_COND] = FL_ZRO;
    }
  else if (reg[r] >> 15) /* a 1 in the left-most bit indicates negative */
    {
      reg[R_COND] = FL_NEG;
    }
  else
    {
      reg[R_COND] = FL_POS;
    }
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
  read += fread (p, sizeof (uint16_t), max_read, file);

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
mem_write (uint16_t address, uint16_t val)
{
  memory[address] = val;
}

void
mem_clear ()
{
  for (int i = 0; i < MEMORY_MAX; i++)
    memory[i] = 0;
}

uint16_t
mem_read (uint16_t address)
{
  if (address == MR_KBSR)
    {
      if (check_key ())
        {
          memory[MR_KBSR] = (1 << 15);
          memory[MR_KBDR] = getchar ();
        }
      else
        {
          memory[MR_KBSR] = 0;
        }
    }
  return memory[address];
}

void
execute_program ()
{
  /* since exactly one condition flag should be set at any given time, set the
   * Z flag */
  reg[R_COND] = FL_ZRO;

  /* set the PC to starting position */
  /* 0x3000 is the default */
  enum
  {
    PC_START = 0x3000
  };
  reg[R_PC] = PC_START;

  int running = 1;
  while (running)
    {
      /* FETCH */
      uint16_t instr = mem_read (reg[R_PC]++);
      uint16_t op = instr >> 12;

      switch (op)
        {
        case OP_ADD:
          {
            /* destination register (DR) */
            uint16_t r0 = (instr >> 9) & 0x7;
            /* first operand (SR1) */
            uint16_t r1 = (instr >> 6) & 0x7;
            /* whether we are in immediate mode */
            uint16_t imm_flag = (instr >> 5) & 0x1;

            if (imm_flag)
              {
                uint16_t imm5 = SIGN_EXT (instr & 0x1F, 5);
                reg[r0] = reg[r1] + imm5;
              }
            else
              {
                uint16_t r2 = instr & 0x7;
                reg[r0] = reg[r1] + reg[r2];
              }

            update_flags (r0);
          }
          break;
        case OP_AND:
          {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t r1 = (instr >> 6) & 0x7;
            uint16_t imm_flag = (instr >> 5) & 0x1;

            if (imm_flag)
              {
                uint16_t imm5 = SIGN_EXT (instr & 0x1F, 5);
                reg[r0] = reg[r1] & imm5;
              }
            else
              {
                uint16_t r2 = instr & 0x7;
                reg[r0] = reg[r1] & reg[r2];
              }
            update_flags (r0);
          }
          break;
        case OP_NOT:
          {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t r1 = (instr >> 6) & 0x7;

            reg[r0] = ~reg[r1];
            update_flags (r0);
          }
          break;
        case OP_BR:
          {
            uint16_t pc_offset = SIGN_EXT (instr & 0x1FF, 9);
            uint16_t cond_flag = (instr >> 9) & 0x7;
            if (cond_flag & reg[R_COND])
              {
                reg[R_PC] += pc_offset;
              }
          }
          break;
        case OP_JMP:
          {
            /* Also handles RET */
            uint16_t r1 = (instr >> 6) & 0x7;
            reg[R_PC] = reg[r1];
          }
          break;
        case OP_JSR:
          {
            uint16_t long_flag = (instr >> 11) & 1;
            reg[R_R7] = reg[R_PC];
            if (long_flag)
              {
                uint16_t long_pc_offset = SIGN_EXT (instr & 0x7FF, 11);
                reg[R_PC] += long_pc_offset; /* JSR */
              }
            else
              {
                uint16_t r1 = (instr >> 6) & 0x7;
                reg[R_PC] = reg[r1]; /* JSRR */
              }
          }
          break;
        case OP_LD:
          {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t pc_offset = SIGN_EXT (instr & 0x1FF, 9);
            reg[r0] = mem_read (reg[R_PC] + pc_offset);
            update_flags (r0);
          }
          break;
        case OP_LDI:
          {
            /* destination register (DR) */
            uint16_t r0 = (instr >> 9) & 0x7;
            /* PCoffset 9*/
            uint16_t pc_offset = SIGN_EXT (instr & 0x1FF, 9);
            /* add pc_offset to the current PC, look at that memory location to
             * get the final address */
            reg[r0] = mem_read (mem_read (reg[R_PC] + pc_offset));
            update_flags (r0);
          }
          break;
        case OP_LDR:
          {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t r1 = (instr >> 6) & 0x7;
            uint16_t offset = SIGN_EXT (instr & 0x3F, 6);
            reg[r0] = mem_read (reg[r1] + offset);
            update_flags (r0);
          }
          break;
        case OP_LEA:
          {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t pc_offset = SIGN_EXT (instr & 0x1FF, 9);
            reg[r0] = reg[R_PC] + pc_offset;
            update_flags (r0);
          }
          break;
        case OP_ST:
          {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t pc_offset = SIGN_EXT (instr & 0x1FF, 9);
            mem_write (reg[R_PC] + pc_offset, reg[r0]);
          }
          break;
        case OP_STI:
          {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t pc_offset = SIGN_EXT (instr & 0x1FF, 9);
            mem_write (mem_read (reg[R_PC] + pc_offset), reg[r0]);
          }
          break;
        case OP_STR:
          {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t r1 = (instr >> 6) & 0x7;
            uint16_t offset = SIGN_EXT (instr & 0x3F, 6);
            mem_write (reg[r1] + offset, reg[r0]);
          }
          break;
        case OP_TRAP:
          reg[R_R7] = reg[R_PC];

          switch (instr & 0xFF)
            {
            case TRAP_GETC:
              /* read a single ASCII char */
              reg[R_R0] = (uint16_t)getchar ();
              update_flags (R_R0);
              break;
            case TRAP_OUT:
              putc ((char)reg[R_R0], stdout);
              fflush (stdout);
              break;
            case TRAP_PUTS:
              {
                /* one char per word */
                uint16_t *c = memory + reg[R_R0];
                while (*c)
                  {
                    putc ((char)*c, stdout);
                    ++c;
                  }
                fflush (stdout);
              }
              break;
            case TRAP_IN:
              {
                printf ("Enter a character: ");
                char c = getchar ();
                putc (c, stdout);
                fflush (stdout);
                reg[R_R0] = (uint16_t)c;
                update_flags (R_R0);
              }
              break;
            case TRAP_PUTSP:
              {
                /* one char per byte (two bytes per word)
                   here we need to swap back to
                   big endian format */
                uint16_t *c = memory + reg[R_R0];
                while (*c)
                  {
                    char char1 = (*c) & 0xFF;
                    putc (char1, stdout);
                    char char2 = (*c) >> 8;
                    if (char2)
                      putc (char2, stdout);
                    ++c;
                  }
                fflush (stdout);
              }
              break;
            case TRAP_HALT:
              // puts ("HALT");
              // fflush (stdout);
              running = 0;
              break;
            }
          break;
        case OP_RES:
        case OP_RTI:
        default:
          abort ();
          break;
        }
    }
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
          if (!read_image (filename))
            {
              printf ("failed to load image: %s\n", filename);
              exit (1);
            }
          execute_program ();
          mem_clear ();
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
