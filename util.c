#include "util.h"
#include "lc3.h"

void
inst_to_bits (char *dest, uint16_t inst)
{
  for (int i = 15; i >= 0; i--)
    {
      dest[i] = (inst % 2) + '0';
      inst /= 2;
    }

  // null-terminate like a good citizen
  dest[16] = 0;
}
