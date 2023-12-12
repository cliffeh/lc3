%{ // TODO move this into one of the code sections below?
#define PPRINT(buf, args...)                \
do {                                        \
  size_t needed = snprintf(0, 0, args) + 1; \
  buf = calloc(needed, sizeof(char));       \
  sprintf(buf, args);                       \
} while(0)
%}

%define api.pure full
%locations
%define parse.error verbose
%param  { program *prog }
%param  { void *scanner }

%union {
  instruction *inst;
  symbol *sym;
  int num;
  char *str;
}

%code requires {
  #include "program.h"
  #include <string.h>
  #include <stdio.h>
  #include <stdlib.h>
  typedef void* yyscan_t;
}

%code provides {
  int parse_program (program *prog, FILE *in);
}

%code {
  // need all of these to prevent compiler warnings
  int yylex_init(yyscan_t *scanner);
  int yyset_in(FILE *in, yyscan_t scanner);
  int yylex_destroy(yyscan_t scanner);
  int yylex(YYSTYPE *yylvalp, YYLTYPE* yyllocp, program *prog, yyscan_t scanner);
  void yyerror (YYLTYPE* yyllocp, program *prog, yyscan_t scanner, const char *msg);

  const char *unescape_string (char *dest, const char *str);
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
  $1->inst |= ($DR << 9) | ($SR1 << 6) | ($SR2 << 0);
  PPRINT($1->pretty, "ADD R%d, R%d, R%d", $DR, $SR1, $SR2);
  $$ = $1;
}
| ADD REG[DR] ',' REG[SR1] ',' NUMLIT[imm5]
{
  $1->inst |= ($DR << 9) | ($SR1 << 6) | (1 << 5) | ($imm5 & 0x001F);
  PPRINT($1->pretty, "ADD R%d, R%d, #%d", $DR, $SR1, $imm5);
  $$ = $1;
}
| AND REG[DR] ',' REG[SR1] ',' REG[SR2]
{
  $1->inst |= ($DR << 9) | ($SR1 << 6) | ($SR2 << 0);
  PPRINT($1->pretty, "AND R%d, R%d, R%d", $DR, $SR1, $SR2);
  $$ = $1;
}
| AND REG[DR] ',' REG[SR1] ',' NUMLIT[imm5]
{
  $1->inst |= ($DR << 9) | ($SR1 << 6) | (1 << 5) | ($imm5 & 0x001F);
  PPRINT($1->pretty, "AND R%d, R%d, #%d", $DR, $SR1, $imm5);
  $$ = $1;
}
| BR LABEL[sym]
{
  $1->inst |= (OP_BR << 12);
  $1->sym = $sym;
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "BR%s%s%s %s",
    ($1->inst & (1<<11)) ? "n": "",
    ($1->inst & (1<<10)) ? "z": "",
    ($1->inst & (1<<9))  ? "p": "",
  $sym->label);
  $$ = $1;
}
| BR NUMLIT[PCoffset9]
{
  $1->inst |= (OP_BR << 12);
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
  $1->inst |= ($BaseR << 6);
  PPRINT($1->pretty, "JMP R%d", $BaseR);
  $$ = $1;
}
| JSR LABEL[sym]
{
  $1->inst |= (1 << 11);
  $1->sym = $sym;
  $1->flags = 0x07FF;
  PPRINT($1->pretty, "JSR %s", $sym->label);
  $$ = $1;
}
| JSRR REG[BaseR]
{
  $1->inst |= ($BaseR << 6);
  PPRINT($1->pretty, "JSRR R%d", $BaseR);
  $$ = $1;
}
| LD REG[DR] ',' LABEL[sym]
{
  $1->inst |= ($DR << 9);
  $1->sym = $sym;
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "LD R%d, %s", $DR, $sym->label);
  $$ = $1;
}
| LDI REG[DR] ',' LABEL[sym]
{
  $1->inst |= ($DR << 9);
  $1->sym = $sym;
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "LDI R%d, %s", $DR, $sym->label);
  $$ = $1;
}
| LDR REG[DR] ',' REG[BaseR] ',' NUMLIT[offset6]
{
  $1->inst |= ($DR << 9) | ($BaseR << 6) | ($offset6 & 0x003F);
  PPRINT($1->pretty, "LDR R%d, R%d, #%d", $DR, $BaseR, $offset6);
  $$ = $1;
}
| LEA REG[DR] ',' LABEL[sym]
{
  $1->inst |= ($DR << 9);
  $1->sym = $sym;
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "LEA R%d, %s", $DR, $sym->label);
  $$ = $1;
}
| NOT REG[DR] ',' REG[SR]
{
  $1->inst |= ($DR << 9) | ($SR << 6) | (0x003F << 0);
  PPRINT($1->pretty, "NOT R%d, R%d", $DR, $SR);
  $$ = $1;
}
| RET
{ // special case of JMP, where R7 is implied as DR
  $1->inst |= (R_R7 << 6);
  PPRINT($1->pretty, "RET");
  $$ = $1;
}
| RTI
{
  PPRINT($1->pretty, "RTI");
  $$ = $1;
}
| ST REG[SR] ',' LABEL[sym]
{
  $1->inst |= ($SR << 9);
  $1->sym = $sym;
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "ST R%d, %s", $SR, $sym->label);
  $$ = $1;
}
| STI REG[SR] ',' LABEL[sym]
{
  $1->inst |= ($SR << 9);
  $1->sym = $sym;
  $1->flags = 0x01FF;
  PPRINT($1->pretty, "STI R%d, %s", $SR, $sym->label);
  $$ = $1;
}
| STR REG[SR] ',' REG[BaseR] ',' NUMLIT[offset6]
{
  $1->inst |= ($SR << 9) | ($BaseR << 6) | ($offset6 & 0x003F);
  PPRINT($1->pretty, "STR R%d, R%d, #%d", $SR, $BaseR, $offset6);
  $$ = $1;
}
| TRAP NUMLIT[trapvect8]
{
 $1->inst |= ($trapvect8 << 0);
  PPRINT($1->pretty, "TRAP x%02X", $trapvect8);
  $$ = $1;
}
/* traps */
| GETC
{
  $1->inst |= (TRAP_GETC << 0);
  PPRINT($1->pretty, "GETC");
  $$ = $1;
}
| OUT
{
  $1->inst |= (TRAP_OUT << 0);
  PPRINT($1->pretty, "OUT");
  $$ = $1;
}
| PUTS
{
  $1->inst |= (TRAP_PUTS << 0);
  PPRINT($1->pretty, "PUTS");
  $$ = $1;
}
| IN
{
  $1->inst |= (TRAP_IN << 0);
  PPRINT($1->pretty, "IN");
  $$ = $1;
}
| PUTSP
{
  $1->inst |= (TRAP_PUTSP << 0);
  PPRINT($1->pretty, "PUTSP");
  $$ = $1;
}
| HALT
{
  $1->inst |= (TRAP_HALT << 0);
  PPRINT($1->pretty, "HALT");
  $$ = $1;
}
/* assembler directives */
| FILL NUMLIT[data]
{
  $1->inst = $data;
  PPRINT($1->pretty, ".FILL x%X", $data);
  $$ = $1;
}
| FILL LABEL[sym]
{
  $1->sym = $sym;
  PPRINT($1->pretty, ".FILL %s", $sym->label);
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

int
parse_program (program *prog, FILE *in)
{
  yyscan_t scanner;
  yylex_init (&scanner);
  yyset_in (in, scanner);

  int rc = yyparse (prog, scanner);

  yylex_destroy (scanner);

  return rc;
}

const char *
unescape_string (char *dest, const char *str)
{
  char *d = dest;
  for (const char *p = str; *p; p++)
    {
      if (*p == '\\')
        {
          switch (*(++p))
            {
              // clang-format off
            case 'a':  *d++ = '\007';  break;
            case 'b':  *d++ = '\b';    break;
            case 'e':  *d++ = '\e';    break;
            case 'f':  *d++ = '\f';    break;
            case 'n':  *d++ = '\n';    break;
            case 'r':  *d++ = '\r';    break;
            case 't':  *d++ = '\t';    break;
            case 'v':  *d++ = '\013';  break;
            case '\\': *d++ = '\\';    break;
            case '?':  *d++ = '?';     break;
            case '\'': *d++ = '\'';    break;
            case '"':  *d++ = '\"';    break;
            default:
              {
                return p;
              }
              // clang-format on
            }
        }
      else
        {
          *d++ = *p;
        }
    }
  // null terminate
  *d = 0;
  return 0;
}

void
yyerror (YYLTYPE* yyllocp, program *prog, yyscan_t scanner, const char *msg)
{
  fprintf(stderr, "[line %d, column %d]: %s\n",
          yyllocp->first_line, yyllocp->first_column, msg);
}
