#ifdef HAVE_CONFIG_H
#include "config.h"
#define VERSION_STRING "lc3vm " PACKAGE_VERSION
#endif

#ifndef VERSION_STRING
#define VERSION_STRING "lc3vm unknown"
#endif

#include "lc3.h"
#include "popt/popt.h"
#include "util.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define MEMORY_MAX (1 << 16)
uint16_t memory[MEMORY_MAX];
uint16_t registers[R_COUNT];

#define SET_COND(result)                                                      \
  do                                                                          \
    {                                                                         \
      if (result == 0)                                                        \
        registers[R_COND] = FL_ZRO;                                           \
      else if (result >> 15)                                                  \
        registers[R_COND] = FL_NEG;                                           \
      else                                                                    \
        registers[R_COND] = FL_POS;                                           \
    }                                                                         \
  while (0)

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

int
main (int argc, const char *argv[])
{
  int rc;
  char *infile = "-", *outfile = "-";
  FILE *in, *out = 0;

  poptContext optCon;

  struct poptOption options[]
      = { /* longName, shortName, argInfo, arg, val, descrip, argDescript */
          { "input", 'i', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &infile,
            'i', "read input from FILE", "FILE" },
          { "output", 'o', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT,
            &outfile, 'o', "write output to FILE", "FILE" },
          { "version", 0, POPT_ARG_NONE, 0, 'Z',
            "show version information and exit", 0 },
          POPT_AUTOHELP POPT_TABLEEND
        };

  optCon = poptGetContext (0, argc, argv, options, 0);

  while ((rc = poptGetNextOpt (optCon)) > 0)
    {
      switch (rc)
        {
        case 'i':
          {
            // TODO support >1 input file?
            if (in)
              {
                ERR_EXIT ("more than one input file specified");
              }
            else if (strcmp (infile, "-") == 0)
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
      ERR_EXIT ("%s: %s\n", poptBadOption (optCon, POPT_BADOPTION_NOALIAS),
                poptStrerror (rc));
    }

  if (!in)
    in = stdin;
  if (!out)
    out = stdout;

  /* the origin tells us where in memory to place the image */
  uint16_t origin;
  size_t read = fread (&origin, sizeof (origin), 1, in);
  origin = swap16 (origin);

  /* we know the maximum file size so we only need one fread */
  uint16_t max_read = MEMORY_MAX - origin;
  uint16_t *p = memory + origin;
  read = fread (p, sizeof (uint16_t), max_read, in);

  /* swap to little endian */
  while (read-- > 0)
    {
      *p = swap16 (*p);
      ++p;
    }

  signal (SIGINT, handle_interrupt);
  disable_input_buffering ();

  registers[R_PC] = origin;

  int running = 1;

  FILE *logfile = fopen ("test.out", "w");

  for (uint16_t inst = memory[registers[R_PC]++]; running;
       inst = memory[registers[R_PC]++])
    {
      fprintf (logfile, "PC: %04X; executing: %04X", registers[R_PC],
               swap16 (inst));
      for (int i = 0; i < 8; i++)
        {
          fprintf (logfile, " R%i: %04X", i, registers[i]);
        }
      fprintf (logfile, "\n");

      switch (GET_OP (inst))
        {
        case OP_ADD:
          {
            /*
            if (bit[5] == 0)
                DR = SR1 + SR2;
            else
                DR = SR1 + SEXT(imm5);
            setcc();
            */
            if (inst & MASK_BIT5)
              registers[inst & MASK_DR]
                  = registers[inst & MASK_SR1] + registers[inst & MASK_SR2];
            else
              registers[inst & MASK_DR] = registers[inst & MASK_SR1]
                                          + sign_extend (inst & MASK_IMM5, 16);

            SET_COND (registers[inst & MASK_DR]);
          }
          break;

        case OP_AND:
          {
            /*
            if (bit[5] == 0)
                DR = SR1 AND SR2;
            else
                DR = SR1 AND SEXT(imm5);
            setcc();
            */
            if (inst & MASK_BIT5)
              registers[inst & MASK_DR]
                  = registers[inst & MASK_SR1] + registers[inst & MASK_SR2];
            else
              registers[inst & MASK_DR] = registers[inst & MASK_SR1]
                                          & sign_extend (inst & MASK_IMM5, 16);

            SET_COND (registers[inst & MASK_DR]);
          }
          break;

        case OP_BR:
          {
            /*
            if ((n AND N) OR (z AND Z) OR (p AND P))
                PC = PC + SEXT(PCoffset9);
            */
            if ((inst & MASK_COND) == 0 // equivalent to nzp
                || ((inst & MASK_COND) & registers[R_COND]))
              {
                registers[R_PC] += sign_extend (inst & MASK_PCOFFSET9, 16);
              }
          }
          break;

        case OP_JMP:
          {
            /* PC = BaseR; */
            registers[R_PC] = registers[inst & MASK_BASER];
          }
          break;

        case OP_JSR:
          {
            /*
            R7 = PC;
            if (bit[11] == 0)
                PC = BaseR;
            else
                PC = PC + SEXT(PCoffset11);
            */
            registers[R_R7] = registers[R_PC];
            if (inst & MASK_BIT11)
              {
                registers[R_PC] += sign_extend (inst & MASK_PCOFFSET11, 16);
              }
            else
              {
                registers[R_PC] = registers[inst & MASK_BASER];
              }
          }
          break;

        case OP_LD:
          {
            /*
            DR = mem[PC + SEXT(PCoffset9)];
            setcc();
            */
            registers[inst & MASK_DR]
                = memory[registers[R_PC]
                         + sign_extend (inst & MASK_PCOFFSET9, 16)];
            SET_COND (registers[inst & MASK_DR]);
          }
          break;

        case OP_LDI:
          {
            /*
            DR = mem[BaseR + SEXT(offset6)];
            setcc();
            */
            registers[inst & MASK_DR]
                = memory[registers[inst & MASK_BASER]
                         + sign_extend (inst & MASK_OFFSET6, 16)];
            SET_COND (registers[inst & MASK_DR]);
          }
          break;

        case OP_LDR:
          {
            /*
            DR = mem[BaseR + SEXT(offset6)];
            setcc();
            */
            registers[inst & MASK_DR]
                = registers[inst & MASK_BASER]
                  + sign_extend (inst & MASK_OFFSET6, 16);
            SET_COND (registers[inst & MASK_DR]);
          }
          break;

        case OP_LEA:
          {
            /*
            DR = PC + SEXT(PCoffset9);
            setcc();
            */
            registers[inst & MASK_DR]
                = registers[R_PC] + sign_extend (inst & MASK_PCOFFSET9, 16);
            SET_COND (registers[inst & MASK_DR]);
          }
          break;

        case OP_NOT:
          {
            /*
            DR = NOT(SR);
            setcc();
            */
            registers[inst & MASK_DR] = ~registers[inst & MASK_SR];
            SET_COND (registers[inst & MASK_DR]);
          }
          break;

        case OP_RTI:
          { /* unused */
            /*
            if (PSR[15] == 0)
                PC = mem[R6]; R6 is the SSP
                R6 = R6+1;
                TEMP = mem[R6];
                R6 = R6+1;
                PSR = TEMP; the privilege mode and condition codes of
                the interrupted process are restored
            else
                Initiate a privilege mode exception;
            */
          }
          break;

        case OP_ST:
          {
            /* mem[PC + SEXT(PCoffset9)] = SR; */
            memory[registers[R_PC] + sign_extend (inst & MASK_PCOFFSET9, 16)]
                = registers[inst & MASK_SR];
          }
          break;

        case OP_STI:
          {
            /* mem[mem[PC + SEXT(PCoffset9)]] = SR; */
            memory[memory[registers[R_PC]
                          + sign_extend (inst & MASK_PCOFFSET9, 16)]]
                = registers[inst & MASK_SR];
          }
          break;

        case OP_STR:
          {
            /* mem[BaseR + SEXT(offset6)] = SR; */
            memory[registers[inst & MASK_BASER]
                   + sign_extend (inst & MASK_OFFSET6, 16)]
                = registers[inst & MASK_SR];
          }
          break;

        case OP_TRAP:
          {
            uint16_t trapvect8 = sign_extend (inst & 0x00FF, 16);
            switch (trapvect8)
              {
              case TRAP_GETC:
                {
                  registers[R_R0] = getchar ();
                  SET_COND (registers[R_R0]);
                }
                break;
              case TRAP_OUT:
                {
                  putc ((char)registers[R_R0], stdout);
                  fflush (stdout);
                }
                break;
              case TRAP_PUTS:
                {
                  uint16_t addr = registers[R_R0];
                  for (char c = memory[addr]; c; c = memory[++addr])
                    {
                      printf ("%c", c);
                    }
                }
                break;
              case TRAP_IN:
                {
                  printf ("Enter a character: ");
                  char c = getchar ();
                  putc (c, stdout);
                  fflush (stdout);
                  registers[R_R0] = (uint16_t)c;
                  SET_COND (registers[R_R0]);
                }
                break;
              case TRAP_PUTSP:
                {
                  /* one char per byte (two bytes per word)
                     here we need to swap back to
                     big endian format */
                  uint16_t *c = memory + registers[R_R0];
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
                {
                  // TODO any other cleanup?
                  running = 0;
                }
                break;
              default:
                {
                  fprintf (stderr, "unknown trap code: %X\n", trapvect8);
                }
              }
          }
          break;

        default:
          {
            fprintf (stderr, "unknown/unimplemented op code!\n");
          }
        }
    }

  restore_input_buffering ();

  exit (0);
}