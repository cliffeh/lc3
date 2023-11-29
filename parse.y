%{
#include "lc3as.h"
#include <stdio.h>
int yylex();    // from our scanner
void yyerror();
extern char *yytext;
%}

%union {
  char *str;
  uint16_t addr;
  int code;
  program prog;
  instruction_list inst_list;
  instruction inst;
}

// operations
%token ADD AND BR JMP JSR JSRR LD LDI LDR
%token LEA NOT RET RTI ST STI STR TRAP

// assembler directives
%token ORIG END STRINGZ

%token <code> REG
%token <addr> HEXLIT
%token <str>  IDENT

%type <prog> program
%type <inst_list> instruction_list
%type <inst> instruction
%type <inst> label
%type <inst> line

%start program

%%

program:
ORIG HEXLIT '\n'
instruction_list
END
 { $$.orig = $2; }
;

instruction_list: {}
/* empty */
| instruction_list line
;

line:
label '\n'
| instruction '\n'
| label instruction '\n'
;

label: IDENT { $$.label = $1; };

instruction:
ADD REG ',' REG ',' REG {}
;


%%

// TODO better error messages
void
yyerror (char const *s)
{
   fprintf (stderr, "%s\n", s);
}
