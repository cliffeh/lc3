%define api.pure full
%locations
%define parse.error verbose
%param       { program *prog }
%param       { void *scanner }

%union {
  instruction *inst;
  symbol *sym;
  int num;
  char *str;
}

%code requires {
  #include "program.h"
  #include "util.h"
  #include <string.h>
  #include <stdio.h>
  #include <stdlib.h>
  typedef void* yyscan_t;
}

%code provides {
  int assemble_program (program *prog, FILE *in);
}

%code {
  // need all of these to prevent compiler warnings
  int yylex_init(yyscan_t *scanner);
  int yyset_in(FILE *in, yyscan_t scanner);
  int yylex_destroy(yyscan_t scanner);
  int yylex(YYSTYPE *yylvalp, YYLTYPE* yyllocp, program *prog, yyscan_t scanner);
  void yyerror (YYLTYPE* yyllocp, program *prog, yyscan_t scanner, const char *msg);
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
%type <sym>  LABEL
%type <num>  NUMLIT REG
%type <str>  STRLIT

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
| LABEL[sym] instruction_list
{
  if($sym->is_set) {
    fprintf(stderr, "error: duplicate symbol: %s\n", $sym->label);
    YYERROR;
  } 
  $sym->addr = $2->addr;
  $sym->is_set = 1;
  // $2->last->next = $3;
  $$ = $2;
}
| instruction instruction_list
{
  $1->last->next = $2;
  $$ = $1;
}
;

instruction:
/* operations */
  ADD REG[DR] ',' REG[SR1] ',' REG[SR2]
{
  $1->word |= ($DR << 9) | ($SR1 << 6) | ($SR2 << 0);
  $$ = $1;
}
| ADD REG[DR] ',' REG[SR1] ',' NUMLIT[imm5]
{
  $1->word |= ($DR << 9) | ($SR1 << 6) | (1 << 5) | ($imm5 & 0x001F);
  $$ = $1;
}
| AND REG[DR] ',' REG[SR1] ',' REG[SR2]
{
  $1->word |= ($DR << 9) | ($SR1 << 6) | ($SR2 << 0);
  $$ = $1;
}
| AND REG[DR] ',' REG[SR1] ',' NUMLIT[imm5]
{
  $1->word |= ($DR << 9) | ($SR1 << 6) | (1 << 5) | ($imm5 & 0x001F);
  $$ = $1;
}
| BR LABEL[sym]
{
  $1->word |= (OP_BR << 12);
  $1->sym = $sym;
  $1->flags = 0x01FF;
  $$ = $1;
}
| BR NUMLIT[PCoffset9]
{
  $1->word |= (OP_BR << 12);
  $1->word |= ($PCoffset9 & 0x01FF);
  $$ = $1;
}
| JMP REG[BaseR]
{
  $1->word |= ($BaseR << 6);
  $$ = $1;
}
| JSR LABEL[sym]
{
  $1->word |= (1 << 11);
  $1->sym = $sym;
  $1->flags = 0x07FF;
  $$ = $1;
}
| JSRR REG[BaseR]
{
  $1->word |= ($BaseR << 6);
  $$ = $1;
}
| LD REG[DR] ',' LABEL[sym]
{
  $1->word |= ($DR << 9);
  $1->sym = $sym;
  $1->flags = 0x01FF;
  $$ = $1;
}
| LDI REG[DR] ',' LABEL[sym]
{
  $1->word |= ($DR << 9);
  $1->sym = $sym;
  $1->flags = 0x01FF;
  $$ = $1;
}
| LDR REG[DR] ',' REG[BaseR] ',' NUMLIT[offset6]
{
  $1->word |= ($DR << 9) | ($BaseR << 6) | ($offset6 & 0x003F);
  $$ = $1;
}
| LEA REG[DR] ',' LABEL[sym]
{
  $1->word |= ($DR << 9);
  $1->sym = $sym;
  $1->flags = 0x01FF;
  $$ = $1;
}
| NOT REG[DR] ',' REG[SR]
{
  $1->word |= ($DR << 9) | ($SR << 6) | (0x003F << 0);
  $$ = $1;
}
| RET
{ // special case of JMP, where R7 is implied as DR
  $1->word |= (R_R7 << 6);
  $$ = $1;
}
| RTI /* $$ = $1 */
| ST REG[SR] ',' LABEL[sym]
{
  $1->word |= ($SR << 9);
  $1->sym = $sym;
  $1->flags = 0x01FF;
  $$ = $1;
}
| STI REG[SR] ',' LABEL[sym]
{
  $1->word |= ($SR << 9);
  $1->sym = $sym;
  $1->flags = 0x01FF;
  $$ = $1;
}
| STR REG[SR] ',' REG[BaseR] ',' NUMLIT[offset6]
{
  $1->word |= ($SR << 9) | ($BaseR << 6) | ($offset6 & 0x003F);
  $$ = $1;
}
| TRAP NUMLIT[trapvect8]
{
  $1->word |= ($trapvect8 << 0);
  $$ = $1;
}
/* traps */
| GETC
{
  $1->word |= (TRAP_GETC << 0);
  $$ = $1;
}
| OUT
{
  $1->word |= (TRAP_OUT << 0);
  $$ = $1;
}
| PUTS
{
  $1->word |= (TRAP_PUTS << 0);
  $$ = $1;
}
| IN
{
  $1->word |= (TRAP_IN << 0);
  $$ = $1;
}
| PUTSP
{
  $1->word |= (TRAP_PUTSP << 0);
  $$ = $1;
}
| HALT
{
  $1->word |= (TRAP_HALT << 0);
  $$ = $1;
}
/* assembler directives */
| FILL NUMLIT[data]
{
  $1->word = $data;
  $$ = $1;
}
| FILL LABEL[sym]
{
  $1->sym = $sym;
  $$ = $1;
}
| STRINGZ STRLIT[raw]
{
  char *escaped = calloc(strlen($raw)+1, sizeof(char));
  const char *test;
  if((test = unescape_string(escaped, $raw)) != 0)
  {
    fprintf(stderr, "error: unknown escape sequence in string literal: \\%c\n", *test);
    YYERROR;
  }

  instruction *inst = $1;
  for(char *p = escaped; *p; p++) {
    inst->word = *p;
    inst->next = calloc(1, sizeof(instruction));
    inst = inst->next;
    inst->addr = prog->len++;
  }
  inst->word = 0;
  free(escaped);
  free($raw);

  $1->last = inst;
  $$ = $1;
}
;

%%

int
assemble_program (program *prog, FILE *in)
{
  yyscan_t scanner;
  yylex_init (&scanner);
  yyset_in (in, scanner);

  int rc = yyparse (prog, scanner);
  if(rc == 0)
    rc = resolve_symbols(prog);

  yylex_destroy (scanner);

  return rc;
}

void
yyerror (YYLTYPE* yyllocp, program *prog, yyscan_t scanner, const char *msg)
{
  fprintf(stderr, "[line %d, column %d]: %s\n",
          yyllocp->first_line, yyllocp->first_column, msg);
}
