%{
#include "lc3as.h"
#include "parser.h"

// TODO set addr
#define NEW_INST(yylval) \
  do { \
    yylval->inst = calloc(1, sizeof(instruction)); \
    yylval->inst->last = yylval->inst; \
  }while(0)
%}

%option noyywrap nounput noinput
%option reentrant bison-bridge

%x STRING
%%
 /* op codes */
[aA][dD][dD]                   { NEW_INST(yylval); return(ADD);     }
[aA][nN][dD]                   { NEW_INST(yylval); return(AND);     }
[bB][rR][nN]?[zZ]?[pP]?        { 
  NEW_INST(yylval);
  for(char *p = yytext+2; *p; p++) {
    switch(*p) {
      case 'n': case 'N': yylval->inst->inst |= (1 << 11); break;
      case 'z': case 'Z': yylval->inst->inst |= (1 << 10); break;
      case 'p': case 'P': yylval->inst->inst |= (1 << 9);  break;
    }
  }
  return(BR);                  }
[jJ][mM][pP]                   { NEW_INST(yylval); return(JMP);     }
[jJ][sS][rR]                   { NEW_INST(yylval); return(JSR);     }
[jJ][sS][rR][rR]               { NEW_INST(yylval); return(JSRR);    }
[lL][dD]                       { NEW_INST(yylval); return(LD);      }
[lL][dD][iI]                   { NEW_INST(yylval); return(LDI);     }
[lL][dD][rR]                   { NEW_INST(yylval); return(LDR);     }
[lL][eE][aA]                   { NEW_INST(yylval); return(LEA);     }
[nN][oO][tT]                   { NEW_INST(yylval); return(NOT);     }
[rR][eE][tT]                   { NEW_INST(yylval); return(RET);     }
[rR][tT][iI]                   { NEW_INST(yylval); return(RTI);     }
[sS][tT]                       { NEW_INST(yylval); return(ST);      }
[sS][tT][iI]                   { NEW_INST(yylval); return(STI);     }
[sS][tT][rR]                   { NEW_INST(yylval); return(STR);     }
[tT][rR][aA][pP]               { NEW_INST(yylval); return(TRAP);    }

 /* trap routines */
[gG][eE][tT][cC]               { NEW_INST(yylval); return(GETC);    }
[oO][uU][tT]                   { NEW_INST(yylval); return(OUT);     }
[pP][uU][tT][sS]               { NEW_INST(yylval); return(PUTS);    }
[iI][nN]                       { NEW_INST(yylval); return(IN);      }
[pP][uU][tT][sS][pP]           { NEW_INST(yylval); return(PUTSP);   }
[hH][aA][lL][tT]               { NEW_INST(yylval); return(HALT);    }


 /* registers */
[rR][0-7]                      { yylval->num = *(yytext+1) - '0'; return(REG); }

 /* assembler directives */
\.[oO][rR][iI][gG]             { return(ORIG);    }
\.[eE][nN][dD]                 { return(END);     }
\.[fF][iI][lL][lL]             { NEW_INST(yylval); return(FILL);    }
\.[sS][tT][rR][iI][nN][gG][zZ] { NEW_INST(yylval); return(STRINGZ); }

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
