%{
#include <stdio.h>
int yylex();
void /*int*/ yyerror();
extern char *yytext;
%}

%token REGISTER
 
%start program

%%

program: REGISTER { printf("parser register: %s\n", yytext); }
;

%%

// TODO better parsing errors
void yyerror (char const *s) {
   fprintf (stderr, "%s\n", s);
 }
