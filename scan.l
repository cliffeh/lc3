%{
#include "program.h"
#include "parse.h"

#define YY_DECL int yylex \
  (                       \
   YYSTYPE *yylval_param, \
   YYLTYPE* yylloc_param, \
   program *prog,         \
   yyscan_t yyscanner     \
  )

#define YY_USER_ACTION                        \
  yylloc->first_line = yylloc->last_line;     \
  yylloc->first_column = yylloc->last_column; \
  if (yylloc->last_line == yylineno)          \
    yylloc->last_column += yyleng;            \
  else {                                      \
    yylloc->last_line = yylineno;             \
    yylloc->last_column = yyleng;             \
  }

%}

%option noyywrap nounput noinput
%option reentrant bison-bridge bison-locations

%x STRING
%%
 /* op codes */
[aA][dD][dD]                   { yylval->num = (OP_ADD << 12); return(ADD);    }
[aA][nN][dD]                   { yylval->num = (OP_AND << 12); return(AND);    }
[bB][rR][nN]?[zZ]?[pP]?        { 
  yylval->num = (OP_BR << 12);
  if(strlen(yytext) == 2) {
    // NB "The assembly language opcode BR is interpreted the same as BRnzp;
    // that is, always branch to the target address."
    yylval->num |= (1 << 11) | (1 << 10) | (1 << 9);
  } else {
    for(char *p = yytext+2; *p; p++) {
      switch(*p) {
        case 'n': case 'N': yylval->num |= (1 << 11); break;
        case 'z': case 'Z': yylval->num |= (1 << 10); break;
        case 'p': case 'P': yylval->num |= (1 << 9);  break;
      }
    }
  }
  return(BR);                  }
[jJ][mM][pP]                   { yylval->num = (OP_JMP << 12);  return(JMP);   }
[jJ][sS][rR]                   { yylval->num = ((OP_JSR << 12) | (1 << 11));   return(JSR);   }
[jJ][sS][rR][rR]               { yylval->num = (OP_JSR << 12);  return(JSRR);  }
[lL][dD]                       { yylval->num = (OP_LD << 12);   return(LD);    }
[lL][dD][iI]                   { yylval->num = (OP_LDI << 12);  return(LDI);   }
[lL][dD][rR]                   { yylval->num = (OP_LDR << 12);  return(LDR);   }
[lL][eE][aA]                   { yylval->num = (OP_LEA << 12);  return(LEA);   }
[nN][oO][tT]                   { yylval->num = (OP_NOT << 12);  return(NOT);   }
[rR][eE][tT]                   { yylval->num = ((OP_JMP << 12) | (R_R7 << 6)); return(RET);   }
[rR][tT][iI]                   { yylval->num = (OP_RTI << 12);  return(RTI);   }
[sS][tT]                       { yylval->num = (OP_ST << 12);   return(ST);    }
[sS][tT][iI]                   { yylval->num = (OP_STI << 12);  return(STI);   }
[sS][tT][rR]                   { yylval->num = (OP_STR << 12);  return(STR);   }
[tT][rR][aA][pP]               { yylval->num = (OP_TRAP << 12); return(TRAP);  }

 /* trap routines */
[gG][eE][tT][cC]               { yylval->num = ((OP_TRAP << 12) | TRAP_GETC);  return(GETC);  }
[oO][uU][tT]                   { yylval->num = ((OP_TRAP << 12) | TRAP_OUT);   return(OUT);   }
[pP][uU][tT][sS]               { yylval->num = ((OP_TRAP << 12) | TRAP_PUTS);  return(PUTS);  }
[iI][nN]                       { yylval->num = ((OP_TRAP << 12) | TRAP_IN);    return(IN);    }
[pP][uU][tT][sS][pP]           { yylval->num = ((OP_TRAP << 12) | TRAP_PUTSP); return(PUTSP); }
[hH][aA][lL][tT]               { yylval->num = ((OP_TRAP << 12) | TRAP_HALT);  return(HALT);  }

 /* registers */
[rR][0-7]                      { yylval->num = *(yytext+1) - '0'; return(REG); }

 /* assembler directives */
\.[oO][rR][iI][gG]             { return(ORIG);    }
\.[eE][nN][dD]                 { return(END);     }
\.[fF][iI][lL][lL]             { return(FILL);    }
\.[sS][tT][rR][iI][nN][gG][zZ] { return(STRINGZ); }

 /* literals */
[xX][0-9a-fA-F]{1,4}           { yylval->num = strtol(yytext+1, 0, 16); return(NUMLIT); }
#(0|-?[1-9][0-9]*)             { yylval->num = strtol(yytext+1, 0, 10); return(NUMLIT); }

\" { // TODO this needs entirely more rigor
  BEGIN STRING;
}
<STRING>(\\\"|[^\"])*          { yylval->str = strdup(yytext); return(STRLIT); }
<STRING>\" {
  BEGIN INITIAL;
}

 /* labels */
[a-zA-Z][_\-a-zA-Z0-9]*        { // NB labels cannot start with a _
  yylval->sym = calloc(1, sizeof(symbol));
  yylval->sym->label = strdup(yytext);
  // hack: capture the current address at the time the label is lexed
  yylval->sym->flags = prog->orig + prog->len;
  return(LABEL);               }

 /* stuff to ignore */
\n      { yylineno++; } // ignore newlines, but count them
[ \t]*  // ignore whitespace
;.*     // ignore comments

 /* if we don't know what it is, just return it */
. return *yytext;
