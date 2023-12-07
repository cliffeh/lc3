%{
#include "lc3as.h"
#include "util.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PPRINT(buf, args...) \
do { \
  size_t needed = snprintf(0, 0, args) + 1; \
  buf = calloc(needed, sizeof(char)); \
  sprintf(buf, args); \
} while(0)

int yylex();
void yyerror();
%}

%define api.pure true
%define parse.error verbose
%parse-param    { program *prog }
%param          { void *scanner }

%union {
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
%token NUMLIT STRLIT LABEL

%type <inst> instruction instruction_list
// special cases of instruction
%type <inst> alloc directive trap
%type <num> NUMLIT REG
%type <str> BR LABEL STRLIT

%start program

%%

program:
  ORIG NUMLIT
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
| LABEL instruction instruction_list
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
  alloc ADD REG[DR] ',' REG[SR1] ',' REG[SR2]
{
  $1->inst = (OP_ADD << 12) | ($DR << 9) | ($SR1 << 6) | ($SR2 << 0);
  PPRINT($1->pretty, "ADD R%d, R%d, R%d", $DR, $SR1, $SR2);
  $$ = $1;
}
| alloc ADD REG[DR] ',' REG[SR1] ',' NUMLIT[imm5]
{
  // TODO is this correct? maybe we should fail if $imm5 is more than 5 bits wide?
  $1->inst = (OP_ADD << 12) | ($DR << 9) | ($SR1 << 6) | (1 << 5) | ($imm5 & 0x001F);
  PPRINT($1->pretty, "ADD R%d, R%d, #%d", $DR, $SR1, $imm5);
  $$ = $1;
}
| alloc AND REG[DR] ',' REG[SR1] ',' REG[SR2]
{
  $1->inst = (OP_AND << 12) | ($DR << 9) | ($SR1 << 6) | ($SR2 << 0);
  PPRINT($1->pretty, "AND R%d, R%d, R%d", $DR, $SR1, $SR2);
  $$ = $1;
}
| alloc AND REG[DR] ',' REG[SR1] ',' NUMLIT[imm5]
{
  // TODO is this correct? maybe we should fail if $imm5 is more than 5 bits wide?
  $1->inst = (OP_AND << 12) | ($DR << 9) | ($SR1 << 6) | (1 << 5) | ($imm5 & 0x001F);
  PPRINT($1->pretty, "AND R%d, R%d, #%d", $DR, $SR1, $imm5);
  $$ = $1;
}
| alloc BR LABEL
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
  $1->sym = find_or_create_symbol(prog, $3, 0, 0);
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "%s %s", $2, $3);
  free($2);
  free($3);
  $$ = $1;
}
| alloc BR NUMLIT[PCoffset9]
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
  // TODO is this correct? maybe we should fail if $PCoffset9 is more than 9 bits wide?
  $1->inst |= ($PCoffset9 & 0x01FF);
  PPRINT($1->pretty, "%s #%d", $2, $PCoffset9);
  free($2);
  $$ = $1;
}
| alloc JMP REG[BaseR]
{
  $1->inst = (OP_JMP << 12) | ($BaseR << 6);
  PPRINT($1->pretty, "JMP R%d", $BaseR);
  $$ = $1;
}
| alloc JSR LABEL
{
  $1->inst = (OP_JSR << 12) | (1 << 11);
  $1->sym = find_or_create_symbol(prog, $3, 0, 0);
  $1->flags = 0x07FF;
  PPRINT($1->pretty, "JSR %s", $3);
  free($3);
  $$ = $1;
}
| alloc JSRR REG[BaseR]
{
  $1->inst = (OP_JSR << 12) | ($BaseR << 6);
  PPRINT($1->pretty, "JSRR R%d", $BaseR);
  $$ = $1;
}
| alloc LD REG[DR] ',' LABEL
{
  $1->inst = (OP_LD << 12) | ($DR << 9);
  $1->sym = find_or_create_symbol(prog, $5, 0, 0);
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "LD R%d, %s", $DR, $5);
  free($5);
  $$ = $1;
}
| alloc LDI REG[DR] ',' LABEL
{
  $1->inst = (OP_LDI << 12) | ($DR << 9);
  $1->sym = find_or_create_symbol(prog, $5, 0, 0);
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "LDI R%d, %s", $DR, $5);
  free($5);
  $$ = $1;
}
| alloc LDR REG[DR] ',' REG[BaseR] ',' NUMLIT[offset6]
{
  // TODO is this correct? maybe we should fail if $offset6 is more than 6 bits wide?
  $1->inst = (OP_LDR << 12) | ($DR << 9) | ($BaseR << 6) | ($offset6 & 0x003F);
  PPRINT($1->pretty, "LDR R%d, R%d, #%d", $DR, $BaseR, $offset6);
  $$ = $1;
}
| alloc LEA REG[DR] ',' LABEL
{
  $1->inst = (OP_LEA << 12) | ($DR << 9);
  $1->sym = find_or_create_symbol(prog, $5, 0, 0);
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "LEA R%d, %s", $DR, $5);
  free($5);
  $$ = $1;
}
| alloc NOT REG[DR] ',' REG[SR]
{
  $1->inst = (OP_NOT << 12) | ($DR << 9) | ($SR << 6) | (0x003F << 0);
  PPRINT($1->pretty, "NOT R%d, R%d", $DR, $SR);
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
| alloc ST REG[SR] ',' LABEL
{
  $1->inst = (OP_ST << 12) | ($SR << 9);
  $1->sym = find_or_create_symbol(prog, $5, 0, 0);
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "ST R%d, %s", $SR, $5);
  free($5);
  $$ = $1;
}
| alloc STI REG[SR] ',' LABEL
{
  $1->inst = (OP_STI << 12) | ($SR << 9);
  $1->sym = find_or_create_symbol(prog, $5, 0, 0);
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "STI R%d, %s", $SR, $5);
  free($5);
  $$ = $1;
}
| alloc STR REG[SR] ',' REG[BaseR] ',' NUMLIT[offset6]
{
  // TODO is this correct? maybe we should fail if $offset6 is more than 6 bits wide?
  $1->inst = (OP_STR << 12) | ($SR << 9) | ($BaseR << 6) | ($offset6 & 0x003F);
  PPRINT($1->pretty, "STR R%d, R%d, #%d", $SR, $BaseR, $offset6);
  $$ = $1;
}
| alloc TRAP NUMLIT[trapvect8]
{
  // TODO is this correct? maybe we should fail if $trapvect8 is more than 8 bits wide?
  $1->inst = (OP_TRAP << 12) | ($trapvect8 << 0);
  PPRINT($1->pretty, "TRAP x%02X", $trapvect8);
  $$ = $1;
}
| directive {$$=$1;}
| trap {$$=$1;}
;

directive:
  alloc FILL NUMLIT[data]
{
  $1->inst = $data;
  PPRINT($1->pretty, ".FILL x%X", $data);
  $$ = $1;
}
| alloc FILL LABEL
{
  $1->sym = find_or_create_symbol(prog, $3, 0, 0);
  PPRINT($1->pretty, ".FILL %s", $3);
  free($3);
  $$ = $1;
}
| alloc STRINGZ STRLIT[raw]
{
  char *escaped = calloc(strlen($raw)+1, sizeof(char));
  if(unescape_string(escaped, $raw) != 0)
  {
    fprintf(stderr, "error: unknown escape sequence in string literal\n");
    YYERROR;
  }

  instruction *inst = $1;
  for(char *p = escaped; *p; p++) {
    inst->inst = *p;
    inst->next = calloc(1, sizeof(instruction));
    inst = inst->next;
    inst->addr = prog->len++;
  }
  inst->inst = 0;
  free(escaped);

  PPRINT($1->pretty, ".STRINGZ \"%s\"", $raw);
  free($raw);

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

%%

// TODO better error messages
void
yyerror (char const *msg)
{
  fprintf(stderr, "parse error: %s\n", msg);
}
