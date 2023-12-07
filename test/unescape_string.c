#include "../util.h"
#include <assert.h>
#include <stdint.h> // for uint16_t
#include <stdio.h>  // for printf
#include <string.h> // for strcmp

int
main ()
{
  int rc = 0;
  char *input_string = "this\\tis\\bmy \\\\test\\n";
  char *expected = "this\tis\bmy \\test\n";
  char output_string[1024];
  const char *test;

  // TODO use rc?
  test = unescape_string(output_string, input_string);
  assert(!test);

  printf("output string: '%s'\n", output_string);
  assert(strcmp(output_string, expected) == 0);

  return 0;
}