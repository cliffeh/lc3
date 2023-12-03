#define MEMORY_MAX (1 << 16)

#include "lc3.h"
#include "util.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uint16_t memory[MEMORY_MAX];
uint16_t registers[R_COUNT];

// TODO put this in util?
uint16_t
sign_extend (uint16_t x, int bit_count)
{
  if ((x >> (bit_count - 1)) & 1)
    {
      x |= (0xFFFF << bit_count);
    }
  return x;
}

int
main (int argc, char *argv)
{
  FILE *in = stdin;

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
        case OP_LEA:
          {
            // 0000 1110 0000 0000
            uint16_t dr = inst & 0x0E00;
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
                        printf("%c", c);
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