%{
#include "parse.h"
%}

%option noyywrap

%%

;.* // ignore comments

[rR][0-7] { printf("register!\n"); return(REGISTER); }

[ \t]* // ignore whitespace
