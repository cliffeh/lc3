#include "../lc3.h"
#include <assert.h>
#include <stdint.h> // for uint16_t
#include <stdio.h>  // for printf
#include <string.h> // for strcmp

void inst_to_bits (char *dest, uint16_t inst);

int
main ()
{
  char buf[32];
  uint16_t inst;

  inst = 0;
  SET_OP (inst, OP_JSR);
  inst_to_bits (buf, inst);
  printf ("%X: %s\n", inst, buf);
  assert (strcmp ("0100000000000000", buf) == 0);
  assert (GET_OP (inst) == OP_JSR);
}