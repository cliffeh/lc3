%{
#include "lc3as.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PPRINT(buf, args...) \
do { \
  size_t needed = snprintf(0, 0, args) + 1; \
  buf = calloc(needed, sizeof(char)); \
  sprintf(buf, args); \
} while(0)

int yylex();
void yyerror();
extern char *yytext;
extern int yylineno;
%}

%parse-param    { program *prog }

%union {
  program *prog;
  instruction *inst;
  int num;
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
%type <inst> instruction instruction_list
// special cases of instruction
%type <inst> alloc directive trap
%type <num> num reg
%type <str> branch label

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

instruction_list:

  /* empty */
{ $$ = 0; }
  // NB this means no labels after the last instruction
| label instruction instruction_list
{
  find_or_create_symbol(prog, $1, $2->addr, 1);
  $2->last->next = $3;
  $$ = $2;
  free($1);
}
| instruction instruction_list
{
  $1->last->next = $2;
  $$ = $1;
}
;

alloc: // hack: allocate instruction storage
  /* empty */
{
  $$ = calloc(1, sizeof(instruction));
  $$->addr = prog->len++;
  $$->last = $$;
}
;

instruction:
  alloc ADD reg ',' reg ',' reg
{
  $1->inst = (OP_ADD << 12) | ($3 << 9) | ($5 << 6) | ($7 << 0);
  PPRINT($1->pretty, "ADD R%d, R%d, R%d", $3, $5, $7);
  $$ = $1;
}
| alloc ADD reg ',' reg ',' num /* imm5 */
{
  // TODO is this correct? maybe we should fail if $7 is more than 5 bits wide?
  $1->inst = (OP_ADD << 12) | ($3 << 9) | ($5 << 6) | (1 << 5) | ($7 & 0x001F);
  PPRINT($1->pretty, "ADD R%d, R%d, %s", $3, $5, yytext);
  $$ = $1;
}
| alloc AND reg ',' reg ',' reg
{
  $1->inst = (OP_AND << 12) | ($3 << 9) | ($5 << 6) | ($7 << 0);
  PPRINT($1->pretty, "AND R%d, R%d, R%d", $3, $5, $7);
  $$ = $1;
}
| alloc AND reg ',' reg ',' num /* imm5 */
{
  // TODO is this correct? maybe we should fail if $7 is more than 5 bits wide?
  $1->inst = (OP_AND << 12) | ($3 << 9) | ($5 << 6) | (1 << 5) | ($7 & 0x001F);
  PPRINT($1->pretty, "AND R%d, R%d, %s", $3, $5, yytext);
  $$ = $1;
}
| alloc branch LABEL
{
  $1->inst = (OP_BR << 12);
  for(char *p = $2+2; *p; p++) {
    switch(*p) {
      case 'n':
      case 'N': $1->inst |= (1 << 11); break;
      case 'z':
      case 'Z': $1->inst |= (1 << 10); break;
      case 'p':
      case 'P': $1->inst |= (1 << 9); break;
    }
  }
  $1->sym = find_or_create_symbol(prog, yytext, 0, 0);
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "%s %s", $2, yytext);
  $$ = $1;
  free($2);
}
| alloc branch num
{
  $1->inst = (OP_BR << 12);
  for(char *p = $2+2; *p; p++) {
    switch(*p) {
      case 'n':
      case 'N': $1->inst |= (1 << 11); break;
      case 'z':
      case 'Z': $1->inst |= (1 << 10); break;
      case 'p':
      case 'P': $1->inst |= (1 << 9); break;
    }
  }
  // TODO is this correct? maybe we should fail if $3 is more than 9 bits wide?
  $1->inst |= ($3 & 0x01FF);
  PPRINT($1->pretty, "%s %s", $2, yytext);
  $$ = $1;
  free($2);
}
| alloc JMP reg
{
  $1->inst = (OP_JMP << 12) | ($3 << 6);
  PPRINT($1->pretty, "JMP R%d", $3);
  $$ = $1;
}
| alloc JSR LABEL
{
  $1->inst = (OP_JSR << 12) | (1 << 11);
  $1->sym = find_or_create_symbol(prog, yytext, 0, 0);
  $1->flags = 0x07FF;
  PPRINT($1->pretty, "JSR %s", yytext);
  $$ = $1;
}
| alloc JSRR reg
{
  $1->inst = (OP_JSR << 12) | ($3 << 6);
  PPRINT($1->pretty, "JSRR R%d", $3);
  $$ = $1;
}
| alloc LD reg ',' LABEL
{
  $1->inst = (OP_LD << 12) | ($3 << 9);
  $1->sym = find_or_create_symbol(prog, yytext, 0, 0);
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "LD R%d, %s", $3, yytext);
  $$ = $1;
}
| alloc LDI reg ',' LABEL
{
  $1->inst = (OP_LDI << 12) | ($3 << 9);
  $1->sym = find_or_create_symbol(prog, yytext, 0, 0);
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "LDI R%d, %s", $3, yytext);
  $$ = $1;
}
| alloc LDR reg ',' reg ',' num /* offset6 */
{
  // TODO is this correct? maybe we should fail if $7 is more than 6 bits wide?
  $1->inst = (OP_LDR << 12) | ($3 << 9) | ($5 << 6) | ($7 & 0x003F);
  PPRINT($1->pretty, "LDR R%d, R%d, %s", $3, $5, yytext);
  $$ = $1;
}
| alloc LEA reg ',' LABEL
{
  $1->inst = (OP_LEA << 12) | ($3 << 9);
  $1->sym = find_or_create_symbol(prog, yytext, 0, 0);
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "LEA R%d, %s", $3, yytext);
  $$ = $1;
}
| alloc NOT reg ',' reg
{
  $1->inst = (OP_NOT << 12) | ($3 << 9) | ($5 << 6) | (0x003F << 0);
  PPRINT($1->pretty, "NOT R%d, R%d", $3, $5);
  $$ = $1;
}
| alloc RET
{
  // special case of JMP, where R7 is implied as DR
  $1->inst = (OP_JMP << 12) | (R_R7 << 6);
  PPRINT($1->pretty, "RET");
  $$ = $1;
}
| alloc RTI
{
  $1->inst = (OP_RTI << 12);
  PPRINT($1->pretty, "RTI");
  $$ = $1;
}
| alloc ST reg ',' LABEL
{
  $1->inst = (OP_ST << 12) | ($3 << 9);
  $1->sym = find_or_create_symbol(prog, yytext, 0, 0);
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "ST R%d, %s", $3, yytext);
  $$ = $1;
}
| alloc STI reg ',' LABEL
{
  $1->inst = (OP_STI << 12) | ($3 << 9);
  $1->sym = find_or_create_symbol(prog, yytext, 0, 0);
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "STI R%d, %s", $3, yytext);
  $$ = $1;
}
| alloc STR reg ',' reg ',' num /* offset6 */
{
  // TODO is this correct? maybe we should fail if $7 is more than 6 bits wide?
  $1->inst = (OP_STR << 12) | ($3 << 9) | ($5 << 6) | ($7 & 0x003F);
  PPRINT($1->pretty, "STR R%d, R%d, %s", $3, $5, yytext);
  $$ = $1;
}
| alloc TRAP num /* trapvect8 */
{
  // TODO is this correct? maybe we should fail if $3 is more than 8 bits wide?
  $1->inst = (OP_TRAP << 12) | ($3 << 0);
  PPRINT($1->pretty, "TRAP %s", yytext);
  $$ = $1;
}
| directive {$$=$1;}
| trap {$$=$1;}
;

directive:
  alloc FILL num
{
  $1->inst = $3;
  PPRINT($1->pretty, ".FILL %s", yytext);
  $$ = $1;
}
| alloc FILL LABEL
{
  $1->sym = find_or_create_symbol(prog, yytext, 0, 0);
  PPRINT($1->pretty, ".FILL %s", yytext);
  $$ = $1;
}
| alloc STRINGZ STRLIT
{
  char *buf = calloc(strlen(yytext)+1, sizeof(char));
  if(unescape_string(buf, yytext) != 0)
  {
    fprintf(stderr, "error: unknown escape sequence in string literal\n");
    YYERROR;
  }

  instruction *inst = $1;
  for(char *p = buf; *p; p++) {
    inst->inst = *p;
    inst->next = calloc(1, sizeof(instruction));
    inst = inst->next;
    inst->addr = prog->len++;
  }
  inst->inst = 0;
  free(buf);

  PPRINT($1->pretty, ".STRINGZ \"%s\"", yytext);

  $1->last = inst;
  $$ = $1;
}
;

trap:
  alloc GETC
{
  $1->inst = (OP_TRAP << 12) | (TRAP_GETC << 0);
  PPRINT($1->pretty, "GETC");
  $$ = $1;
}
| alloc OUT
{
  $1->inst = (OP_TRAP << 12) | (TRAP_OUT << 0);
  PPRINT($1->pretty, "OUT");
  $$ = $1;
}
| alloc PUTS
{
  $1->inst = (OP_TRAP << 12) | (TRAP_PUTS << 0);
  PPRINT($1->pretty, "PUTS");
  $$ = $1;
}
| alloc IN
{
  $1->inst = (OP_TRAP << 12) | (TRAP_IN << 0);
  PPRINT($1->pretty, "IN");
  $$ = $1;
}
| alloc PUTSP
{
  $1->inst = (OP_TRAP << 12) | (TRAP_PUTSP << 0);
  PPRINT($1->pretty, "PUTSP");
  $$ = $1;
}
| alloc HALT
{
  $1->inst = (OP_TRAP << 12) | (TRAP_HALT << 0);
  PPRINT($1->pretty, "HALT");
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
  $$ = *(yytext+1) - '0';
}
;

branch: BR
{
  $$ = strdup(yytext);
}
;

label: LABEL
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
