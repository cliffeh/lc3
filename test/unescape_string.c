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

  // TODO use rc?
  rc = unescape_string(output_string, input_string);

  printf("output string: '%s'\n", output_string);

  return strcmp(output_string, expected);
}