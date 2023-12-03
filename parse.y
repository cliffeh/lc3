%{
#include "lc3as.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int yylex();    // from our scanner
void yyerror();
extern char *yytext;
extern int yylineno;

// some convenience macros for operations
#define OP_NARG(dest, code) \
  dest = calloc(1, sizeof(instruction)); \
  dest->pos = prog->len++; \
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

%parse-param    { program *prog }

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

// trap routines
%token GETC OUT PUTS IN PUTSP HALT

// registers
%token REG

// assembler directives
%token ORIG END FILL STRINGZ

// literals
%token DECLIT HEXLIT STRLIT LABEL

%type program
%type <inst_list> instruction_list
%type <inst> instruction
// special cases of instruction
%type <inst> directive label trap
%type <num> num imm5 offset6 trapvect8
%type <reg> reg
%type <str> branch

%start program

%%

program:
  ORIG num
  instruction_list
  END
{
  prog->orig = $2;
  prog->instructions = $3;
}
;

/*
  this would be simpler with right recursion, but:
    1) we want to be able to capture address instructions, and
    2) we want to protect against blowing out our stack if
       the input program is large
*/
instruction_list:
  /* empty */
{ $$ = 0; }
| instruction
{
  $$ = calloc(1, sizeof(instruction_list));
  $$->head = $1;
  $$->tail = 0;
  $$->last = $$;
}
| instruction_list instruction
{
  $1->last->tail = calloc(1, sizeof(instruction_list));
  $1->last->tail->head = $2;
  $1->last = $1->last->tail;
  $$ = $1;
}
;

instruction:
  ADD reg ',' reg ',' reg
{
  OP_3ARG($$, OP_ADD, reg[0], $2, reg[1], $4, reg[2], $6);
  $$->immediate = 0;
}
| ADD reg ',' reg ',' imm5
{
  OP_3ARG($$, OP_ADD, reg[0], $2, reg[1], $4, imm5, $6);
  $$->immediate = 1;
}
| AND reg ',' reg ',' reg
{
  OP_3ARG($$, OP_AND, reg[0], $2, reg[1], $4, reg[2], $6);
  $$->immediate = 0;
}
| AND reg ',' reg ',' imm5
{
  OP_3ARG($$, OP_AND, reg[0], $2, reg[1], $4, imm5, $6);
  $$->immediate = 1;
}
| branch LABEL
{
  OP_1ARG($$, OP_BR, label, strdup(yytext));
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
  $$->immediate = 1;
}
| JSR LABEL
{
  OP_1ARG($$, OP_JSR, label, strdup(yytext));
  $$->immediate = 1;
}
| JSRR reg
{
  OP_1ARG($$, OP_JSR, reg[0], $2);
  $$->immediate = 0;
}
| LD reg ',' LABEL
{
  OP_2ARG($$, OP_LD, reg[0], $2, label, strdup(yytext));
}
| LDI reg ',' LABEL
{
  OP_2ARG($$, OP_LDI, reg[0], $2, label, strdup(yytext));
}
| LDR reg ',' reg ',' offset6
{
  OP_3ARG($$, OP_LDR, reg[0], $2, reg[1], $4, offset6, $6);
}
| LEA reg ',' LABEL
{
  OP_2ARG($$, OP_LEA, reg[0], $2, label, strdup(yytext));
}
| NOT reg ',' reg
{
  OP_2ARG($$, OP_NOT, reg[0], $2, reg[1], $4);
}
| RET
{
  // special case of JMP, where R7 is implied as DR
  OP_1ARG($$, OP_JMP, reg[0], char_to_reg('7'));
}
| RTI
{
  OP_NARG($$, OP_RTI);
}
| ST reg ',' LABEL
{
  OP_2ARG($$, OP_ST, reg[0], $2, label, strdup(yytext));
}
| STI reg ',' LABEL
{
  OP_2ARG($$, OP_STI, reg[0], $2, label, strdup(yytext));
}
| STR reg ',' reg ',' offset6
{
  OP_3ARG($$, OP_STR, reg[0], $2, reg[1], $4, offset6, $6);
}
| TRAP trapvect8
{
  OP_1ARG($$, OP_TRAP, trapvect8, $2);
  $$->immediate = 1;
}
| directive
| trap
| label
;

directive:
  FILL num
{
  $$ = calloc(1, sizeof(instruction));
  $$->inst = $2;
  $$->op = -3;
  $$->immediate = 1;
  $$->pos = prog->len++;
}
| FILL LABEL
{
  $$ = calloc(1, sizeof(instruction));
  $$->label = strdup(yytext);
  $$->op = -3;
  $$->immediate = 0;
  $$->pos = prog->len++;
}
|
  STRINGZ STRLIT
{
  $$ = calloc(1, sizeof(instruction));
  // TODO a more sophisticated way of trimming quotes
  $$->label = strdup(yytext+1);
  $$->label[strlen($$->label)-1] = 0;
  $$->op = -2;
  $$->pos = prog->len;
  prog->len += (strlen($$->label)+1);
}
;

label: LABEL
{
  $$ = calloc(1, sizeof(instruction));
  $$->label = strdup(yytext);
  $$->op = -1;
  $$->pos = prog->len;
}
;

/*
   convention: set the immediate flag to distinguish between
   these and TRAP 0xnn
*/
trap:
  GETC
{
  OP_1ARG($$, OP_TRAP, trapvect8, 0x20);
}
| OUT
{
  OP_1ARG($$, OP_TRAP, trapvect8, 0x21);
}
| PUTS
{
  OP_1ARG($$, OP_TRAP, trapvect8, 0x22);
}
| IN
{
  OP_1ARG($$, OP_TRAP, trapvect8, 0x23);
}
| PUTSP
{
  OP_1ARG($$, OP_TRAP, trapvect8, 0x24);
}
| HALT
{
  OP_1ARG($$, OP_TRAP, trapvect8, 0x25);
}
;

imm5:
  num
{
  if($1 < -16 || $1 > 15) {
    fprintf(stderr, "error: imm5 value out of range: %d\n", $1);
    YYERROR;
  }
  $$ = $1;
}
;

offset6:
  num
{
  if($1 < -32 || $1 > 31) {
    fprintf(stderr, "error: offset6 value out of range: %d\n", $1);
    YYERROR;
  }
  $$ = $1;
}
;

trapvect8:
  num
{
  if($1 < 0 || $1 > 255) {
    fprintf(stderr, "error: trapvect8 value out of range: %d\n", $1);
    YYERROR;
  }
  $$ = $1;
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
  fprintf(stderr, "parse error on line %d: %s : %s\n", yylineno, s, yytext);
}
