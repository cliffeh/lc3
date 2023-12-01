%{
#include "lc3as.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int yylex();    // from our scanner
void yyerror();
extern char *yytext;

// some convenience macros for operations
#define OP_NARG(dest, code) \
  dest = calloc(1, sizeof(instruction)); \
  dest->op = code
#define OP_1ARG(dest, code, d1, a1) \
  OP_NARG(dest, code); \
  dest->d1 = a1
#define OP_2ARG(dest, code, d1, a1, d2, a2) \
  OP_1ARG(dest, code, d1, a1); \
  dest->d2 = a2
#define OP_3ARG(dest, code, d1, a1, d2, a2, d3, a3) \
  OP_2ARG(dest, code, d1, a1, d2, a2); \
  dest->d3 = a3
%}

%parse-param {program *prog}

%union {
  program *prog;
  instruction_list *inst_list;
  instruction *inst;
  int num;
  int reg;
  char *str;
}

// operations
%token ADD AND BR JMP JSR JSRR LD LDI LDR
%token LEA NOT RET RTI ST STI STR TRAP

// registers
%token REG

// assembler directives
%token ORIG END STRINGZ

// literals
%token DECLIT HEXLIT LABEL

%type <prog> program
%type <inst_list> instruction_list
%type <inst> instruction
%type <num> num
%type <reg> reg
%type <str> branch
%type <str> label

%start program

%%

program:
  ORIG num
  instruction_list
  END
{
  $$ = prog; // return value
  $$->orig = $2;
  $$->instructions = $3;
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
  LABEL // label
{
  $$ = calloc(1, sizeof(instruction));
  $$->label = strdup(yytext);
  // special case for labels
  $$->op = -1;
}
| ADD reg ',' reg ',' reg
{
  OP_3ARG($$, OP_ADD, reg[0], $2, reg[1], $4, reg[2], $6);
  $$->immediate = 0;
}
| ADD reg ',' reg ',' num
{
  OP_3ARG($$, OP_ADD, reg[0], $2, reg[1], $4, imm5, $6);
  $$->immediate = 1;
}
| AND reg ',' reg ',' reg
{
  OP_3ARG($$, OP_AND, reg[0], $2, reg[1], $4, reg[2], $6);
  $$->immediate = 0;
}
| AND reg ',' reg ',' num
{
  OP_3ARG($$, OP_AND, reg[0], $2, reg[1], $4, imm5, $6);
  $$->immediate = 1;
}
| branch label
{
  OP_1ARG($$, OP_BR, label, $2);
  for(char *p = $1+2; *p; p++) {
    switch(*p) {
      case 'n':
      case 'N': $$->cond |= FL_NEG; break;
      case 'z':
      case 'Z': $$->cond |= FL_ZRO; break;
      case 'p':
      case 'P': $$->cond |= FL_POS; break;
    }
  }
}
| JMP reg
{
  OP_1ARG($$, OP_JMP, reg[0], $2);
  // as a convention, we'll use the immediate flag
  // to distinguish between JMP and RET
  $$->immediate = 0;
}
| JSR label
{
  OP_1ARG($$, OP_JSR, label, $2);
  $$->immediate = 1;
}
| JSRR reg
{
  OP_1ARG($$, OP_JSR, reg[0], $2);
  $$->immediate = 0;
}
| LD reg ',' label
{
  OP_2ARG($$, OP_LD, reg[0], $2, label, $4);
}
| LDI reg ',' label
{
  OP_2ARG($$, OP_LDI, reg[0], $2, label, $4);
}
| LDR reg ',' reg ',' num
{
  OP_3ARG($$, OP_LDR, reg[0], $2, reg[1], $4, offset6, $6);
}
| LEA reg ',' label
{
  OP_2ARG($$, OP_LEA, reg[0], $2, label, $4);
}
| NOT reg ',' reg
{
  OP_2ARG($$, OP_NOT, reg[0], $2, reg[1], $4);
}
| RET
{
  // special case of JMP, where R7 is implied as DR
  OP_1ARG($$, OP_JMP, reg[0], char_to_reg('7'));
  // as a convention, we'll use the immediate flag
  // to distinguish between JMP and RET
  $$->immediate = 1;
}
| RTI
{
  OP_NARG($$, OP_RTI);
}
| ST reg ',' label
{
  OP_2ARG($$, OP_ST, reg[0], $2, label, $4);
}
| STI reg ',' label
{
  OP_2ARG($$, OP_STI, reg[0], $2, label, $4);
}
| STR reg ',' reg ',' num
{
  OP_3ARG($$, OP_STR, reg[0], $2, reg[1], $4, offset6, $6);
}
;

num:
  HEXLIT
{
  $$ = strtol(yytext+1, 0, 16);
}
| DECLIT
{
  $$ = strtol(yytext+1, 0, 10);
}
;

reg: REG
{
  $$ = char_to_reg(*(yytext+1));
}
;

label: LABEL
{
  $$ = strdup(yytext);
}
;

branch: BR
{
  $$ = strdup(yytext);
}
;

%%

// TODO better error messages
void
yyerror (char const *s)
{
  fprintf (stderr, "%s\n", s);
}
