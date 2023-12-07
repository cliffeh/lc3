%{
#include "lc3as.h"
#include "parser.h"
%}

%option noyywrap nounput noinput
%option reentrant bison-bridge

%x STRING
%%
 /* op codes */
[aA][dD][dD]                   { return(ADD);     }
[aA][nN][dD]                   { return(AND);     }
[bB][rR][nN]?[zZ]?[pP]?        { yylval->str = strdup(yytext); return(BR); }
[jJ][mM][pP]                   { return(JMP);     }
[jJ][sS][rR]                   { return(JSR);     }
[jJ][sS][rR][rR]               { return(JSRR);    }
[lL][dD]                       { return(LD);      }
[lL][dD][iI]                   { return(LDI);     }
[lL][dD][rR]                   { return(LDR);     }
[lL][eE][aA]                   { return(LEA);     }
[nN][oO][tT]                   { return(NOT);     }
[rR][eE][tT]                   { return(RET);     }
[rR][tT][iI]                   { return(RTI);     }
[sS][tT]                       { return(ST);      }
[sS][tT][iI]                   { return(STI);     }
[sS][tT][rR]                   { return(STR);     }
[tT][rR][aA][pP]               { return(TRAP);    }

 /* trap routines */
[gG][eE][tT][cC]               { return(GETC);    }
[oO][uU][tT]                   { return(OUT);     }
[pP][uU][tT][sS]               { return(PUTS);    }
[iI][nN]                       { return(IN);      }
[pP][uU][tT][sS][pP]           { return(PUTSP);   }
[hH][aA][lL][tT]               { return(HALT);    }


 /* registers */
[rR][0-7]                      { yylval->num = *(yytext+1) - '0'; return(REG); }

 /* assembler directives */
\.[oO][rR][iI][gG]             { return(ORIG);    }
\.[eE][nN][dD]                 { return(END);     }
\.[fF][iI][lL][lL]             { return(FILL);    }
\.[sS][tT][rR][iI][nN][gG][zZ] { return(STRINGZ); }

 /* literals */
[xX][0-9a-fA-F]{1,4}           { yylval->num = strtol(yytext+1, 0, 16); return(NUMLIT);  }
#(0|-?[1-9][0-9]*)             { yylval->num = strtol(yytext+1, 0, 10); return(NUMLIT);  }

\" { // TODO this needs entirely more rigor
  BEGIN STRING;
}
<STRING>(\\\"|[^\"])*          { yylval->str = strdup(yytext); return(STRLIT); }
<STRING>\" {
  BEGIN INITIAL;
}

 /* labels */
[_a-zA-Z][_\-a-zA-Z0-9]*       { yylval->str = strdup(yytext); return(LABEL); }

 /* other considerations */
, { return(','); } // comma-separated arguments
[ \t]*             // ignore whitespace
\n { yylineno++; } // ignore newlines, but count lines
;.*                // ignore comments

. { fprintf(stderr, "unexpected input on line %d\n", yylineno); }
