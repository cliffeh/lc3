%define api.pure full
%locations
%define parse.error verbose
%param       { program *prog }
%param       { void *scanner }

%union {
  int num;
  char *str;
  symbol *sym;
}

%code requires {
  #include "program.h"
  #include "util.h"
  #include <string.h>
  #include <stdio.h>
  #include <stdlib.h>
  typedef void* yyscan_t;

  #define ADDR(prog) prog->orig + prog->len
}

%code provides {
  // need all of these to prevent compiler warnings
  int  yylex_init (yyscan_t *scanner);
  void yyset_in (FILE *in, yyscan_t scanner);
  int  yylex_destroy (yyscan_t scanner);
}

%code {
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

%type <num> ADD AND BR JMP JSR JSRR LD LDI LDR
%type <num> LEA NOT RET RTI ST STI STR TRAP
%type <num> GETC OUT PUTS IN PUTSP HALT
%type <num> NUMLIT REG
%type <str> STRLIT
%type <sym> LABEL

%type <num> r3 r2offset6 r2 r1lab r1 r0lab r0

%start program

%%

program:
  preamble
  instruction_list
  END
;

preamble:
  ORIG NUMLIT[orig]
{
  prog->orig = $orig;
}
;

instruction_list:
  instruction_or_label
| instruction_or_label instruction_list
;

instruction_or_label:
instruction
| LABEL[sym] instruction
{
  for(int i = prog->orig; i < ADDR(prog); i++)
    {
      if(prog->symbols[i] && strcmp($sym->label, prog->symbols[i]->label) == 0)
        {
          fprintf(stderr, "error: duplicate label: %s\n", $sym->label);
          YYERROR;
        }
    }

  if(prog->symbols[ADDR(prog)])
    {
      // TODO allow multiple labels per address?
      fprintf(stderr, "error: more than one label declared for address: %d", ADDR(prog));
      YYERROR;
    }

  // hack: we're depending on our lexer to capture the address at the time the label is lexed
  prog->symbols[$sym->flags] = $sym;
}
;

r3:        ADD | AND; // 3 registers
r2offset6: LDR | STR; // 2 registers and an offset6
r2:        NOT;       // 2 registers
r1lab:     LD  | LDI | LEA | ST | STI;
r1:        JMP | JSRR;
r0lab:     BR; // JSR
r0:        RET | RTI | GETC | OUT | PUTS | IN | PUTSP | HALT;

instruction:
/* operations */
  r3[op] REG[DR] ',' REG[SR1] ',' REG[SR2]
{
  prog->memory[ADDR(prog)++] = $op | ($DR << 9) | ($SR1 << 6) | ($SR2 << 0);
}
| r3[op] REG[DR] ',' REG[SR1] ',' NUMLIT[imm5]
{
  prog->memory[ADDR(prog)++] = $op | ($DR << 9) | ($SR1 << 6) | (1 << 5) | ($imm5 & 0x001F);
}
| r2offset6[op] REG[DR] ',' REG[BaseR] ',' NUMLIT[offset6]
{
  prog->memory[ADDR(prog)++] = $op | ($DR << 9) | ($BaseR << 6) | ($offset6 & 0x003F);
}
| r2[op] REG[DR] ',' REG[SR]
{
  prog->memory[ADDR(prog)++] = $op | ($DR << 9) | ($SR << 6) | (0x003F << 0);
}
| r1lab[op] REG[DR] ',' LABEL[ref]
{
  prog->memory[ADDR(prog)] = $op | ($DR << 9);
  $ref->flags = 0x1FF; // PCoffset9
  prog->ref[ADDR(prog)++] = $ref;
}
| r1lab[op] REG[DR] ',' NUMLIT[PCoffset9]
{
  prog->memory[ADDR(prog)] = $op | ($DR << 9) | ($PCoffset9 & 0x01FF);
}
| r1[op] REG[BaseR]
{
  prog->memory[ADDR(prog)++] = $op | ($BaseR << 6);
}
| r0lab[op] LABEL[ref]
{
  prog->memory[ADDR(prog)] = $op;
  $ref->flags = 0x1FF; // PCoffset9
  prog->ref[ADDR(prog)++] = $ref;
}
| r0lab[op] NUMLIT[PCoffset9]
{
  prog->memory[ADDR(prog)++] = $op | ($PCoffset9 & 0x01FF);
}
| r0[op]
{
  prog->memory[ADDR(prog)++] = $op;
}
| JSR[op] LABEL[ref]
{
  prog->memory[ADDR(prog)] = $op;
  $ref->flags = 0x7FF; // PCoffset11
  prog->ref[ADDR(prog)++] = $ref;
}
| TRAP[op] NUMLIT[trapvect8]
{
  prog->memory[ADDR(prog)++] = $op | ($trapvect8 << 0);
}
/* assembler directives */
| FILL NUMLIT[data]
{
  // type hint to the disassembler
  symbol *ref = calloc(1, sizeof(symbol));
  ref->flags = (HINT_FILL << 12);
  prog->ref[ADDR(prog)] = ref;
  prog->memory[ADDR(prog)++] = $data;
}
| FILL LABEL[ref]
{
  $ref->flags = (HINT_FILL << 12);
  prog->ref[ADDR(prog)++] = $ref;
}
| STRINGZ STRLIT[raw]
{
  // hint to the disassembler
  symbol *ref = calloc(1, sizeof(symbol));
  ref->flags = (HINT_STRINGZ << 12);
  prog->ref[ADDR(prog)] = ref;

  char *escaped = calloc(strlen($raw)+1, sizeof(char));
  const char *test;
  if((test = unescape_string(escaped, $raw)) != 0)
    {
      fprintf(stderr, "error: unknown escape sequence in string literal: \\%c\n", *test);
      YYERROR;
    }

  for(char *p = escaped; *p; p++) 
    {
      prog->memory[prog->orig + prog->len++] = *p;
    }
  prog->memory[prog->orig + prog->len++] = 0; // null-terminate
  free(escaped);
  free($raw);
}
;

%%

void
yyerror (YYLTYPE* yyllocp, program *prog, yyscan_t scanner, const char *msg)
{
  fprintf(stderr, "[line %d, column %d]: %s\n",
          yyllocp->first_line, yyllocp->first_column, msg);
}
