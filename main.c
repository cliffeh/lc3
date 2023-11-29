#include <stdio.h>
#include <stdlib.h>

extern int yylex();

int
main (int argc, char *argv[])
{
  yylex();
  printf ("Hello, World!\n");

  exit (0);
}
