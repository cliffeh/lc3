%{
#include "lc3as.h"
#include "parser.h"

#define NEW_INST (calloc(1, sizeof(instruction)))
%}

%option noyywrap nounput noinput
%option reentrant bison-bridge

%x STRING
%%
 /* op codes */
[aA][dD][dD]                   { yylval->inst = NEW_INST; return(ADD);     }
[aA][nN][dD]                   { yylval->inst = NEW_INST; return(AND);     }
[bB][rR][nN]?[zZ]?[pP]?        { 
  yylval->inst = NEW_INST;
  for(char *p = yytext+2; *p; p++) {
    switch(*p) {
      case 'n': case 'N': yylval->inst->inst |= (1 << 11); break;
      case 'z': case 'Z': yylval->inst->inst |= (1 << 10); break;
      case 'p': case 'P': yylval->inst->inst |= (1 << 9);  break;
    }
  }
  return(BR);                  }
[jJ][mM][pP]                   { yylval->inst = NEW_INST; return(JMP);     }
[jJ][sS][rR]                   { yylval->inst = NEW_INST; return(JSR);     }
[jJ][sS][rR][rR]               { yylval->inst = NEW_INST; return(JSRR);    }
[lL][dD]                       { yylval->inst = NEW_INST; return(LD);      }
[lL][dD][iI]                   { yylval->inst = NEW_INST; return(LDI);     }
[lL][dD][rR]                   { yylval->inst = NEW_INST; return(LDR);     }
[lL][eE][aA]                   { yylval->inst = NEW_INST; return(LEA);     }
[nN][oO][tT]                   { yylval->inst = NEW_INST; return(NOT);     }
[rR][eE][tT]                   { yylval->inst = NEW_INST; return(RET);     }
[rR][tT][iI]                   { yylval->inst = NEW_INST; return(RTI);     }
[sS][tT]                       { yylval->inst = NEW_INST; return(ST);      }
[sS][tT][iI]                   { yylval->inst = NEW_INST; return(STI);     }
[sS][tT][rR]                   { yylval->inst = NEW_INST; return(STR);     }
[tT][rR][aA][pP]               { yylval->inst = NEW_INST; return(TRAP);    }

 /* trap routines */
[gG][eE][tT][cC]               { yylval->inst = NEW_INST; return(GETC);    }
[oO][uU][tT]                   { yylval->inst = NEW_INST; return(OUT);     }
[pP][uU][tT][sS]               { yylval->inst = NEW_INST; return(PUTS);    }
[iI][nN]                       { yylval->inst = NEW_INST; return(IN);      }
[pP][uU][tT][sS][pP]           { yylval->inst = NEW_INST; return(PUTSP);   }
[hH][aA][lL][tT]               { yylval->inst = NEW_INST; return(HALT);    }


 /* registers */
[rR][0-7]                      { yylval->num = *(yytext+1) - '0'; return(REG); }

 /* assembler directives */
\.[oO][rR][iI][gG]             { return(ORIG);    }
\.[eE][nN][dD]                 { return(END);     }
\.[fF][iI][lL][lL]             { yylval->inst = NEW_INST; return(FILL);    }
\.[sS][tT][rR][iI][nN][gG][zZ] { yylval->inst = NEW_INST; return(STRINGZ); }

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
