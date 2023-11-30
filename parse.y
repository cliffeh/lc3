%{
#include "lc3as.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int yylex();    // from our scanner
void yyerror();
extern char *yytext;
%}

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
  (*prog)->instructions = $3;
}
;

instruction_list:
/* empty */
{ $$ = 0; }
| instruction instruction_list
{
  $$ = calloc(1, sizeof(instruction_list));
  $$->head = $1;
  $$->tail = $2;
}
;

instruction:
IDENT
{
  $$ = calloc(1, sizeof(instruction));
  $$->line_label = strdup(yytext);
}
| ADD REG ',' REG ',' REG {}
;

%%

// TODO better error messages
void
yyerror (char const *s)
{
   fprintf (stderr, "%s\n", s);
}
