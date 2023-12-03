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

uint16_t
sign_extend (uint16_t x, int bit_count)
{
  if ((x >> (bit_count - 1)) & 1)
    {
      x |= (0xFFFF << bit_count);
    }
  return x;
}

uint16_t swap16(uint16_t x)
{
    return (x << 8) | (x >> 8);
}
