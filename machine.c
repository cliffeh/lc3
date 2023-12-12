#include "machine.h"

int
load_machine (machine *vm, FILE *in)
{
  /* the origin tells us where in memory to place the image */
  uint16_t origin;
  size_t read = fread (&origin, sizeof (origin), 1, in);
  origin = SWAP16 (origin);

  /* we know the maximum file size so we only need one fread */
  uint16_t *p = vm->memory + origin;
  read = fread (p, sizeof (uint16_t), (MEMORY_MAX - origin), in);

  /* swap to little endian */
  while (read-- > 0)
    {
      *p = SWAP16 (*p);
      ++p;
    }

  // TODO check for read errors?
  return 0;
}
