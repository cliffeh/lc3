%{
#include "lc3as.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int yylex();    // from our scanner
void yyerror();
extern char *yytext;
extern int yylineno;

#define PRETTY_PRINT(out, inst, flags, fmt, ...) \
do{ \
  if(flags & FORMAT_HEX) \
    fprintf(out, "%s%04x", (flags & FORMAT_ADDR) ? "  " : "", swap16(inst)); \
  if(flags & FORMAT_BITS) { \
    char buf[32]; \
    inst_to_bits(buf, inst); \
    fprintf(out, "%s%s", (flags & (FORMAT_ADDR|FORMAT_HEX)) ? "  " : "", buf); \
  } \
  if(flags & FORMAT_PRETTY) \
    fprintf(out, "  " fmt "\n" __VA_OPT__(,) __VA_ARGS__); \
 }while(0)

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

%parse-param    { program *prog }{ int flags }{ FILE *out }

%union {
  program *prog;
  instruction *inst;
  symbol *sym;
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

%type program preamble postamble
%type <inst> instruction instruction_list
// special cases of instruction
%type <inst> alloc directive trap
%type <sym> label
%type <num> number imm5 offset6 trapvect8
%type <reg> reg
%type <str> branch string

%start program

%%

program:
  preamble
  instruction_list
  postamble
{
  prog->instructions = $2;
}
;

preamble: ORIG number
{
  if (flags & FORMAT_PRETTY)
    fprintf(out, ".ORIG x%04X\n", $2);
  prog->orig = $2;
}
;

postamble:
  END
{
  if (flags & FORMAT_PRETTY)
    fprintf(out, ".END\n");
}
;

instruction_list:
  /* empty */
{ $$ = 0; }
| label instruction_list
{
  $1->next = prog->symbols;
  prog->symbols = $1;
  $$ = $2;
}
| instruction instruction_list
{
  $1->next = $2;
  $$ = $1;
}
;

// TODO keep symbol table separate from instructions
label: LABEL
{
  $$ = calloc(1, sizeof(symbol));
  $$->label = strdup(yytext);
  $$->pos = prog->len;
  if(flags & FORMAT_PRETTY)
    fprintf(out, "%s%s\n", (flags & FORMAT_ADDR) ? "  ": "", yytext);
}
;

alloc: // hack: allocate instruction storage
  /* empty */
{
  $$ = calloc(1, sizeof(instruction));
  $$->pos = prog->len++;
  if(flags & FORMAT_ADDR)
    // NB the assembler expects addresses starting at 0,
    // but for pretty-printing it's nicer to index them at
    // 1 to account for ORIG at the top of the program
    fprintf(out, "%04x", prog->len);
}
;

instruction:
  alloc ADD reg ',' reg ',' reg
{
  $1->inst = (OP_ADD << 12) | ($3 << 9) | ($5 << 6) | ($7 << 0);
  PRETTY_PRINT(out, $1->inst, flags, "ADD R%d, R%d, %s", $3, $5, yytext);
  $$ = $1;
}
| alloc ADD reg ',' reg ',' imm5
{
  $1->inst = (OP_ADD << 12) | ($3 << 9) | ($5 << 6) | (1 << 5) | ($7 << 0);
  PRETTY_PRINT(out, $1->inst, flags, "ADD R%d, R%d, %s", $3, $5, yytext);
  $$ = $1;
}
| alloc AND reg ',' reg ',' reg
{
  $1->inst = (OP_AND << 12) | ($3 << 9) | ($5 << 6) | ($7 << 0);
  PRETTY_PRINT(out, $1->inst, flags, "AND R%d, R%d, R%d", $3, $5, $7);
  $$ = $1;
}
| alloc AND reg ',' reg ',' imm5
{
  $1->inst = (OP_AND << 12) | ($3 << 9) | ($5 << 6) | (1 << 5) | ($7 << 0);
  PRETTY_PRINT(out, $1->inst, flags, "AND R%d, R%d, %s", $3, $5, yytext);
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
  $1->label = strdup(yytext);
  $1->flags = 0x01FF;
  PRETTY_PRINT(out, $1->inst, flags, "%s %s", $2, yytext);
  $$ = $1;
}
| alloc branch number
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
  $1->inst |= ($3 & 0x01FF); // TODO is this correct? maybe we should fail if $3 is more than 9 bits wide?
  PRETTY_PRINT(out, $1->inst, flags, "%s %s", $2, yytext);
  $$ = $1;
}
| alloc JMP reg
{
  $1->inst = (OP_JMP << 12) | ($3 << 6);
  PRETTY_PRINT(out, $1->inst, flags, "JMP R%d", $3);
  $$ = $1;
}
| alloc JSR LABEL
{
  $1->inst = (OP_JSR << 12) | (1 << 11);
  $1->label = strdup(yytext);
  $1->flags = 0x07FF;
  PRETTY_PRINT(out, $1->inst, flags, "JSR %s", yytext);
  $$ = $1;
}
| alloc JSRR reg
{
  $1->inst = (OP_JSR << 12) | ($3 << 6);
  PRETTY_PRINT(out, $1->inst, flags, "JSRR R%d", $3);
  $$ = $1;
}
| alloc LD reg ',' LABEL
{
  $1->inst = (OP_LD << 12) | ($3 << 9);
  $1->label = strdup(yytext);
  $1->flags = 0x01FF;

  PRETTY_PRINT(out, $1->inst, flags, "LD R%d, %s", $3, yytext);
  $$ = $1;
}
| alloc LDI reg ',' LABEL
{
  $1->inst = (OP_LDI << 12) | ($3 << 9);
  $1->label = strdup(yytext);
  $1->flags = 0x01FF;
  PRETTY_PRINT(out, $1->inst, flags, "LDI R%d, %s", $3, yytext);
  $$ = $1;
}
| alloc LDR reg ',' reg ',' offset6
{
  $1->inst = (OP_LDR << 12) | ($3 << 9) | ($5 << 6) | ($7 & 0x002F); // TODO is this correct? maybe we should fail if $7 is more than 6 bits wide?
  PRETTY_PRINT(out, $1->inst, flags, "LDR R%d, R%d, %s", $3, $5, yytext);
  $$ = $1;
}
| alloc LEA reg ',' LABEL
{
  $1->inst = (OP_LEA << 12) | ($3 << 9);
  $1->label = strdup(yytext);
  $1->flags = 0x01FF;
  PRETTY_PRINT(out, $1->inst, flags, "LEA R%d, %s", $3, yytext);
  $$ = $1;
}
| alloc NOT reg ',' reg
{
  $1->inst = (OP_NOT << 12) | ($3 << 9) | ($5 << 6) | (0x002F << 0);
  PRETTY_PRINT(out, $1->inst, flags, "NOT R%d, R%d", $3, $5);
  $$ = $1;
}
| alloc RET
{
  // special case of JMP, where R7 is implied as DR
  $1->inst = (OP_JMP << 12) | (R_R7 << 6);
  PRETTY_PRINT(out, $1->inst, flags, "RET");
  $$ = $1;
}
| alloc RTI
{
  $1->inst = (OP_RTI << 12);
  PRETTY_PRINT(out, $1->inst, flags, "RTI");
  $$ = $1;
}
| alloc ST reg ',' LABEL
{
  $1->inst = (OP_ST << 12) | ($3 << 9);
  $1->label = strdup(yytext);
  $1->flags = 0x01FF;
  PRETTY_PRINT(out, $1->inst, flags, "ST R%d, %s", $3, yytext);
  $$ = $1;
}
| alloc STI reg ',' LABEL
{
  $1->inst = (OP_STI << 12) | ($3 << 9);
  $1->label = strdup(yytext);
  $1->flags = 0x01FF;
  PRETTY_PRINT(out, $1->inst, flags, "STI R%d, %s", $3, yytext);
  $$ = $1;
}
| alloc STR reg ',' reg ',' offset6
{
  $1->inst = (OP_STR << 12) | ($3 << 9) | ($5 << 6) | ($7 & 0x002F); // TODO is this correct? maybe we should fail if $7 is more than 6 bits wide?
  PRETTY_PRINT(out, $1->inst, flags, "STR R%d, R%d, %s", $3, $5, yytext);
  $$ = $1;
}
| alloc TRAP trapvect8
{
  $1->inst = (OP_TRAP << 12) | ($3 << 0);
  PRETTY_PRINT(out, $1->inst, flags, "TRAP %s", yytext);
  $$ = $1;
}
| directive {$$=$1;}
| trap {$$=$1;}
;

directive:
  alloc FILL number
{
  $1->inst = $3;
  if(flags & FORMAT_PRETTY)
    fprintf(out, "  .FILL %s\n", yytext);
  $$ = $1;
}
| alloc FILL LABEL
{
  $1->label = strdup(yytext);
  if(flags & FORMAT_PRETTY)
    fprintf(out, "  .FILL %s\n", yytext);
  $$ = $1;
}
| alloc STRINGZ string
{
  // TODO trim quotes in the lexer
  char *str = $3+1;
  str[strlen(str)-1] = 0;

  char *buf = calloc(strlen(str)+1, sizeof(char));
  if(unescape_string(buf, str) != 0)
  {
    fprintf(stderr, "error: unknown escape sequence in string literal\n");
    YYERROR;
  }

  instruction *inst = $1;
  for(char *p = buf; *p; p++) {
    // printf("appending %c\n", *p);
    inst->inst = *p;
    inst->next = calloc(1, sizeof(instruction));
    inst = inst->next;
    inst->pos = prog->len++;
  }
  inst->inst = 0;
  free(buf);

  // for(inst = $1; inst; inst = inst->next)
  //  printf("reading back: %c\n", inst->inst);

  if(flags & FORMAT_PRETTY)
    fprintf(out, "  .STRINGZ \"%s\"\n", str);

  $$ = $1;
}
;

trap:
  alloc GETC
{
  $1->inst = (OP_TRAP << 12) | (TRAP_GETC << 0);
  PRETTY_PRINT(out, $1->inst, flags, "GETC");
  $$ = $1;
}
| alloc OUT
{
  $1->inst = (OP_TRAP << 12) | (TRAP_OUT << 0);
  PRETTY_PRINT(out, $1->inst, flags, "OUT");
  $$ = $1;
}
| alloc PUTS
{
  $1->inst = (OP_TRAP << 12) | (TRAP_PUTS << 0);
  PRETTY_PRINT(out, $1->inst, flags, "PUTS");
  $$ = $1;
}
| alloc IN
{
  $1->inst = (OP_TRAP << 12) | (TRAP_IN << 0);
  PRETTY_PRINT(out, $1->inst, flags, "IN");
  $$ = $1;
}
| alloc PUTSP
{
  $1->inst = (OP_TRAP << 12) | (TRAP_PUTSP << 0);
  PRETTY_PRINT(out, $1->inst, flags, "PUTSP");
  $$ = $1;
}
| alloc HALT
{
  $1->inst = (OP_TRAP << 12) | (TRAP_HALT << 0);
  PRETTY_PRINT(out, $1->inst, flags, "HALT");
  $$ = $1;
}
;

imm5:
  number
{
  // TODO check for no more than 5 bits
  // if($1 < -16 || $1 > 15) {
  //   fprintf(stderr, "error: imm5 value out of range: %d\n", $1);
  //   YYERROR;
  // }
  $$ = $1;
}
;

offset6:
  number
{
  // TODO check for no more than 6 bits
  // if($1 < -32 || $1 > 31) {
  //   fprintf(stderr, "error: offset6 value out of range: %d\n", $1);
  //   YYERROR;
  // }
  $$ = $1;
}
;

trapvect8:
  number
{
  // TODO check for no more than 8 bits
  // if($1 < 0 || $1 > 255) {
  //   fprintf(stderr, "error: trapvect8 value out of range: %d\n", $1);
  //   YYERROR;
  // }
  $$ = $1;
}
;

number:
  HEXLIT
{
  $$ = strtol(yytext+1, 0, 16);
}
| DECLIT
{
  $$ = strtol(yytext+1, 0, 10);
}
;

string:
  STRLIT
{
  $$ = strdup(yytext);
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
