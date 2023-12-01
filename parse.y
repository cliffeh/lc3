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
  LABEL // label
{
  $$ = calloc(1, sizeof(instruction));
  $$->label = strdup(yytext);
  // special case for labels
  $$->op = -1;
}
| ADD reg ',' reg ',' reg
{
  $$ = calloc(1, sizeof(instruction));
  $$->op = OP_ADD;
  $$->dr =  $2;
  $$->sr1 = $4;
  $$->sr2 = $6;
  $$->immediate = 0;
}
| ADD reg ',' reg ',' num
{
  $$ = calloc(1, sizeof(instruction));
  $$->op = OP_ADD;
  $$->dr =  $2;
  $$->sr1 = $4;
  $$->imm5 = $6;
  $$->immediate = 1;
}
| AND reg ',' reg ',' reg
{
  $$ = calloc(1, sizeof(instruction));
  $$->op = OP_AND;
  $$->dr =  $2;
  $$->sr1 = $4;
  $$->sr2 = $6;
  $$->immediate = 0;
}
| AND reg ',' reg ',' num
{
  $$ = calloc(1, sizeof(instruction));
  $$->op = OP_AND;
  $$->dr =  $2;
  $$->sr1 = $4;
  $$->imm5 = $6;
  $$->immediate = 1;
}
| branch label
{
  $$ = calloc(1, sizeof(instruction));
  $$->op = OP_BR;
  $$->label = $2;
  for(char *p = $1; *p; p++) {
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
  $$ = calloc(1, sizeof(instruction));
  $$->op = OP_JMP;
  $$->dr = $2;
  // as a convention, we'll use the immediate flag
  // to distinguish between JMP and RET
  $$->immediate = 0;
}
| JSR label
{
  $$ = calloc(1, sizeof(instruction));
  $$->op = OP_JSR;
  $$->label = $2;
  $$->immediate = 1;
}
| JSRR reg
{
  $$ = calloc(1, sizeof(instruction));
  $$->op = OP_JSR;
  $$->dr = $2;
  $$->immediate = 0;
}
| LD reg ',' label
{
  $$ = calloc(1, sizeof(instruction));
  $$->op = OP_LD;
  $$->dr = $2;
  $$->label = $4;
}
| LDI reg ',' label
{
  $$ = calloc(1, sizeof(instruction));
  $$->op = OP_LDI;
  $$->dr = $2;
  $$->label = $4;
}
| LDR reg ',' reg ',' num
{
  $$ = calloc(1, sizeof(instruction));
  $$->op = OP_LDR;
  $$->dr = $2;
  $$->sr1 = $4;
  $$->offset6 = $6;
}
| LEA reg ',' label
{
  $$ = calloc(1, sizeof(instruction));
  $$->op = OP_LEA;
  $$->dr = $2;
  $$->label = $4;
}
| NOT reg ',' reg
{
  $$ = calloc(1, sizeof(instruction));
  $$->op = OP_NOT;
  $$->dr = $2;
  $$->sr1 = $4;
}
| RET
{
  $$ = calloc(1, sizeof(instruction));
  $$->op = OP_JMP;
  // special case of JMP, where R7 is implied as DR
  $$->dr = char_to_reg('7');
  // as a convention, we'll use the immediate flag
  // to distinguish between JMP and RET
  $$->immediate = 1;
}
| RTI
{
  $$ = calloc(1, sizeof(instruction));
  $$->op = OP_RTI;
}
| ST reg ',' label
{
  $$ = calloc(1, sizeof(instruction));
  $$->op = OP_ST;
  $$->sr1 = $2;
  $$->label = $4;
}
| STI reg ',' label
{
  $$ = calloc(1, sizeof(instruction));
  $$->op = OP_STI;
  $$->sr1 = $2;
  $$->label = $4;
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
