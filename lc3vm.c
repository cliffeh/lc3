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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_MAX (1 << 16)

// 1111 0000 0000 0000
#define OP_MASK 0xF000
// 0000 1110 0000 0000
#define DR_MASK 0x0E00
// 0000 0001 1100 0000
#define SR1_MASK 0x01C0
#define BASER_MASK SR1_MASK
// 0000 0000 0000 0111
#define SR2_MASK 0x0007
// 0000 0000 0001 0000
#define IMM_MASK 0x0010
// 0000 0000 0001 1111
#define IMM5_MASK 0x001F
// 1000 0000 0000 0000
#define NEG_MASK 0x8000

#define SET_COND(result)                                                      \
  do                                                                          \
    {                                                                         \
      if (result == 0)                                                        \
        registers[R_COND] = FL_ZRO;                                           \
      else if (result & NEG_MASK)                                             \
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

uint16_t memory[MEMORY_MAX];
uint16_t registers[R_COUNT];

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

  registers[R_PC] = origin;

  for (uint16_t inst = memory[registers[R_PC]++]; /* anything to check? */;
       inst = memory[registers[R_PC]++])
    {
      switch (GET_OP (inst))
        {
        case OP_ADD:
          {
            uint16_t dr = inst & DR_MASK;
            uint16_t sr1 = inst & SR1_MASK;
            uint16_t result
                = registers[sr1]
                  + ((inst & IMM_MASK) ? sign_extend (inst & IMM5_MASK, 16)
                                       : registers[inst & SR2_MASK]);
            registers[dr] = result;
            SET_COND (result);
          }
          break;

        case OP_AND:
          {
            uint16_t dr = inst & DR_MASK;
            uint16_t sr1 = inst & SR1_MASK;
            uint16_t result
                = registers[dr]
                  & ((inst & IMM_MASK) ? sign_extend (inst & IMM5_MASK, 16)
                                       : registers[inst & SR2_MASK]);
            registers[dr] = result;
            SET_COND (result);
          }
          break;

        case OP_LEA:
          {
            // 0000 1110 0000 0000
            uint16_t dr = inst & DR_MASK;
            // PCoffset9
            uint16_t offset = sign_extend (inst & 0x01FF, 16);
            registers[dr] = registers[R_PC] + offset;
          }
          break;

        case OP_TRAP:
          {
            uint16_t trapvect8 = sign_extend (inst & 0x00FF, 16);
            switch (trapvect8)
              {
              case TRAP_PUTS:
                {
                  uint16_t addr = registers[R_R0];
                  for (char c = memory[addr]; c; c = memory[++addr])
                    {
                      printf ("%c", c);
                    }
                }
                break;
              case TRAP_HALT:
                {
                  // TODO probably something a little more sophisticated than
                  // just exiting
                  exit (0);
                }
                break;
              }
          }
          break;
        }
    }
}