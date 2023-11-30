%{
#include "lc3as.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int yylex();    // from our scanner
void yyerror();
extern char *yytext;
%}

/* Generate the parser description file. */
%verbose
/* Enable run-time traces (yydebug). */
%define parse.trace

%parse-param {program **prog}

%union {
  char *str;
  uint16_t addr;
  int code;
  instruction_list *inst_list;
  instruction *inst;
}

// operations
%token ADD AND BR JMP JSR JSRR LD LDI LDR
%token LEA NOT RET RTI ST STI STR TRAP

// assembler directives
%token ORIG END STRINGZ

%token <code> REG
%token <addr> HEXLIT
%token IDENT

%type <inst_list> instruction_list
%type <inst> instruction

%start program

%%

program:
ORIG HEXLIT
instruction_list
END
{
  *prog = calloc(1, sizeof(program));
  (*prog)->orig = $2;
}
;

instruction_list:
/* empty */
| instruction instruction_list
;

instruction:
IDENT
| IDENT instruction
| ADD REG ',' REG ',' REG
;

%%

// TODO better error messages
void
yyerror (char const *s)
{
   fprintf (stderr, "%s\n", s);
}
