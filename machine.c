#include "machine.h"

int
load_image (uint16_t *memory, FILE *in)
{
  /* the origin tells us where in memory to place the image */
  uint16_t orig;
  size_t read = fread (&orig, sizeof (orig), 1, in);
  orig = SWAP16 (orig);

  /* we know the maximum file size so we only need one fread */
  uint16_t *p = memory + orig;
  read = fread (p, sizeof (uint16_t), (MEMORY_MAX - orig), in);

  /* swap to little endian */
  while (read-- > 0)
    {
      *p = SWAP16 (*p);
      ++p;
    }

  // TODO check for read errors?
  return 0;
}
