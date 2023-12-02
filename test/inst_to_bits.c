#include "../util.h"
#include <assert.h>
#include <stdint.h> // for uint16_t
#include <stdio.h>  // for printf
#include <string.h> // for strcmp

int
main ()
{
  char buf[32];
  uint16_t inst;

  inst = 0xF000;
  inst_to_bits (buf, inst);
  printf ("%X: %s\n", inst, buf);
  assert (strcmp ("1111000000000000", buf) == 0);

  inst = 12345;
  inst_to_bits (buf, inst);
  printf ("%X: %s\n", inst, buf);
  assert (strcmp ("0011000000111001", buf) == 0);
}