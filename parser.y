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
%type <inst> ADD AND BR JMP JSR JSRR LD LDI LDR
%type <inst> LEA NOT RET RTI ST STI STR TRAP
%type <inst> GETC OUT PUTS IN PUTSP HALT
%type <inst> FILL STRINGZ
%type <num> NUMLIT REG
%type <str> LABEL STRLIT

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
| LABEL[label] instruction instruction_list
{
  find_or_create_symbol(prog, $label, $2->addr, 1);
  free($label);
  $2->last->next = $3;
  $$ = $2;
}
| instruction instruction_list
{
  if(!$1) fprintf(stderr, "INSTRUCTION IS NULL\n");
  if(!$2) fprintf(stderr, "LIST IS NULL\n");
  $1->last->next = $2;
  $$ = $1;
}
;

instruction:
  ADD REG[DR] ',' REG[SR1] ',' REG[SR2]
{
  $1->inst = (OP_ADD << 12) | ($DR << 9) | ($SR1 << 6) | ($SR2 << 0);
  PPRINT($1->pretty, "ADD R%d, R%d, R%d", $DR, $SR1, $SR2);
  $$ = $1;
}
| ADD REG[DR] ',' REG[SR1] ',' NUMLIT[imm5]
{
  // TODO is this correct? maybe we should fail if $imm5 is more than 5 bits wide?
  $1->inst = (OP_ADD << 12) | ($DR << 9) | ($SR1 << 6) | (1 << 5) | ($imm5 & 0x001F);
  PPRINT($1->pretty, "ADD R%d, R%d, #%d", $DR, $SR1, $imm5);
  $$ = $1;
}
| AND REG[DR] ',' REG[SR1] ',' REG[SR2]
{
  $1->inst = (OP_AND << 12) | ($DR << 9) | ($SR1 << 6) | ($SR2 << 0);
  PPRINT($1->pretty, "AND R%d, R%d, R%d", $DR, $SR1, $SR2);
  $$ = $1;
}
| AND REG[DR] ',' REG[SR1] ',' NUMLIT[imm5]
{
  // TODO is this correct? maybe we should fail if $imm5 is more than 5 bits wide?
  $1->inst = (OP_AND << 12) | ($DR << 9) | ($SR1 << 6) | (1 << 5) | ($imm5 & 0x001F);
  PPRINT($1->pretty, "AND R%d, R%d, #%d", $DR, $SR1, $imm5);
  $$ = $1;
}
| BR LABEL[label]
{ // NB nzp flags set by scanner
  $1->inst |= (OP_BR << 12);
  $1->sym = find_or_create_symbol(prog, $label, 0, 0);
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "BR%s%s%s %s",
    ($1->inst & (1<<11)) ? "n": "",
    ($1->inst & (1<<10)) ? "z": "",
    ($1->inst & (1<<9))  ? "p": "",
  $2);
  free($label);
  $$ = $1;
}
| BR NUMLIT[PCoffset9]
{ // NB nzp flags set by scanner
  $1->inst |= (OP_BR << 12);
  // TODO is this correct? maybe we should fail if $PCoffset9 is more than 9 bits wide?
  $1->inst |= ($PCoffset9 & 0x01FF);
  PPRINT($1->pretty, "BR%s%s%s #%d",
    ($1->inst & (1<<11)) ? "n": "",
    ($1->inst & (1<<10)) ? "z": "",
    ($1->inst & (1<<9))  ? "p": "",
  $PCoffset9);
  $$ = $1;
}
| JMP REG[BaseR]
{
  $1->inst = (OP_JMP << 12) | ($BaseR << 6);
  PPRINT($1->pretty, "JMP R%d", $BaseR);
  $$ = $1;
}
| JSR LABEL[label]
{
  $1->inst = (OP_JSR << 12) | (1 << 11);
  $1->sym = find_or_create_symbol(prog, $label, 0, 0);
  $1->flags = 0x07FF;
  PPRINT($1->pretty, "JSR %s", $label);
  free($label);
  $$ = $1;
}
| JSRR REG[BaseR]
{
  $1->inst = (OP_JSR << 12) | ($BaseR << 6);
  PPRINT($1->pretty, "JSRR R%d", $BaseR);
  $$ = $1;
}
| LD REG[DR] ',' LABEL[label]
{
  $1->inst = (OP_LD << 12) | ($DR << 9);
  $1->sym = find_or_create_symbol(prog, $label, 0, 0);
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "LD R%d, %s", $DR, $label);
  free($label);
  $$ = $1;
}
| LDI REG[DR] ',' LABEL[label]
{
  $1->inst = (OP_LDI << 12) | ($DR << 9);
  $1->sym = find_or_create_symbol(prog, $label, 0, 0);
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "LDI R%d, %s", $DR, $label);
  free($label);
  $$ = $1;
}
| LDR REG[DR] ',' REG[BaseR] ',' NUMLIT[offset6]
{
  // TODO is this correct? maybe we should fail if $offset6 is more than 6 bits wide?
  $1->inst = (OP_LDR << 12) | ($DR << 9) | ($BaseR << 6) | ($offset6 & 0x003F);
  PPRINT($1->pretty, "LDR R%d, R%d, #%d", $DR, $BaseR, $offset6);
  $$ = $1;
}
| LEA REG[DR] ',' LABEL[label]
{
  $1->inst = (OP_LEA << 12) | ($DR << 9);
  $1->sym = find_or_create_symbol(prog, $label, 0, 0);
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "LEA R%d, %s", $DR, $label);
  free($label);
  $$ = $1;
}
| NOT REG[DR] ',' REG[SR]
{
  $1->inst = (OP_NOT << 12) | ($DR << 9) | ($SR << 6) | (0x003F << 0);
  PPRINT($1->pretty, "NOT R%d, R%d", $DR, $SR);
  $$ = $1;
}
| RET
{
  // special case of JMP, where R7 is implied as DR
  $1->inst = (OP_JMP << 12) | (R_R7 << 6);
  PPRINT($1->pretty, "RET");
  $$ = $1;
}
| RTI
{
  $1->inst = (OP_RTI << 12);
  PPRINT($1->pretty, "RTI");
  $$ = $1;
}
| ST REG[SR] ',' LABEL[label]
{
  $1->inst = (OP_ST << 12) | ($SR << 9);
  $1->sym = find_or_create_symbol(prog, $label, 0, 0);
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "ST R%d, %s", $SR, $label);
  free($label);
  $$ = $1;
}
| STI REG[SR] ',' LABEL[label]
{
  $1->inst = (OP_STI << 12) | ($SR << 9);
  $1->sym = find_or_create_symbol(prog, $label, 0, 0);
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "STI R%d, %s", $SR, $label);
  free($label);
  $$ = $1;
}
| STR REG[SR] ',' REG[BaseR] ',' NUMLIT[offset6]
{
  // TODO is this correct? maybe we should fail if $offset6 is more than 6 bits wide?
  $1->inst = (OP_STR << 12) | ($SR << 9) | ($BaseR << 6) | ($offset6 & 0x003F);
  PPRINT($1->pretty, "STR R%d, R%d, #%d", $SR, $BaseR, $offset6);
  $$ = $1;
}
| TRAP NUMLIT[trapvect8]
{
  // TODO is this correct? maybe we should fail if $trapvect8 is more than 8 bits wide?
  $1->inst = (OP_TRAP << 12) | ($trapvect8 << 0);
  PPRINT($1->pretty, "TRAP x%02X", $trapvect8);
  $$ = $1;
}
// traps
| GETC
{
  $1->inst = (OP_TRAP << 12) | (TRAP_GETC << 0);
  PPRINT($1->pretty, "GETC");
  $$ = $1;
}
| OUT
{
  $1->inst = (OP_TRAP << 12) | (TRAP_OUT << 0);
  PPRINT($1->pretty, "OUT");
  $$ = $1;
}
| PUTS
{
  $1->inst = (OP_TRAP << 12) | (TRAP_PUTS << 0);
  PPRINT($1->pretty, "PUTS");
  $$ = $1;
}
| IN
{
  $1->inst = (OP_TRAP << 12) | (TRAP_IN << 0);
  PPRINT($1->pretty, "IN");
  $$ = $1;
}
| PUTSP
{
  $1->inst = (OP_TRAP << 12) | (TRAP_PUTSP << 0);
  PPRINT($1->pretty, "PUTSP");
  $$ = $1;
}
| HALT
{
  $1->inst = (OP_TRAP << 12) | (TRAP_HALT << 0);
  PPRINT($1->pretty, "HALT");
  $$ = $1;
}
// assembler directives
| FILL NUMLIT[data]
{
  $1->inst = $data;
  PPRINT($1->pretty, ".FILL x%X", $data);
  $$ = $1;
}
| FILL LABEL[label]
{
  $1->sym = find_or_create_symbol(prog, $label, 0, 0);
  PPRINT($1->pretty, ".FILL %s", $label);
  free($label);
  $$ = $1;
}
| STRINGZ STRLIT[raw]
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

%%

// TODO better error messages
void
yyerror (char const *msg)
{
  fprintf(stderr, "parse error: %s\n", msg);
}
