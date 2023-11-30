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
  instruction_list *inst_list;
  instruction *inst;
  uint16_t addr;
  int reg;
}

// operations
%token ADD AND BR JMP JSR JSRR LD LDI LDR
%token LEA NOT RET RTI ST STI STR TRAP

// registers
%token REG

// assembler directives
%token ORIG END STRINGZ

// literals
%token DECLIT HEXLIT IDENT

%type <inst_list> instruction_list
%type <inst> instruction
%type <addr> integer
%type <reg> register

%start program

%%

program:
  ORIG integer
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
  IDENT // label
{
  $$ = calloc(1, sizeof(instruction));
  $$->label = strdup(yytext);
  // special case for labels
  $$->op = -1;
}
| ADD register ',' register ',' register
{
  $$ = calloc(1, sizeof(instruction));
  $$->op = OP_ADD;
  $$->dr =  $2;
  $$->sr1 = $4;
  $$->sr2 = $6;
  $$->immediate = 0;
}
| ADD register ',' register ',' integer
{
  $$ = calloc(1, sizeof(instruction));
  $$->op = OP_ADD;
  $$->dr =  $2;
  $$->sr1 = $4;
  $$->imm5 = $6;
  $$->immediate = 1;
}
;

integer:
  HEXLIT
{
  // parse the literal to a uint16_t
  $$ = strtol(yytext+1, 0, 16);
}
| DECLIT
{
  $$ = atoi(yytext+1);
}
;

register: REG
{
  $$ = char_to_reg(*(yytext+1));
}
;

%%

// TODO better error messages
void
yyerror (char const *s)
{
  fprintf (stderr, "%s\n", s);
}
