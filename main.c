#include <stdio.h>
#include <stdlib.h>

// extern int yylex();
extern int yyparse();

int
main (int argc, char *argv[])
{
  // yylex();
  yyparse();
  printf ("Hello, World!\n");

  exit (0);
}
