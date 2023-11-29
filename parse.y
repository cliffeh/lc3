%{
#include <stdio.h>
int yylex();
void /*int*/ yyerror();
extern char *yytext;
%}

// operations
%token ADD AND BR JMP JSR JSRR LD LDI LDR
%token LEA NOT RET RTI ST STI STR TRAP

// registers
%token REG

// assembler directives
%token ORIG END STRINGZ

%token IDENT
 
%start program

%%

program: REG { printf("parser register: %s\n", yytext); }
;

%%

// TODO better parsing errors
void yyerror (char const *s) {
   fprintf (stderr, "%s\n", s);
}
