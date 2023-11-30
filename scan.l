%{
#include "lc3as.h"
#include "parse.h"
%}

%option noyywrap

%%

;.* // ignore comments

[aA][dD][dD]     { return(ADD);  }
[aA][nN][dD]     { return(AND);  }
[bB][rR](n|z|p|zp|np|nz|nzp)?    {
                   return(BR);   }
[jJ][mM][pP]     { return(JMP);  }
[jJ][sS][rR]     { return(JSR);  }
[jJ][sS][rR][rR] { return(JSRR); }
[lL][dD]         { return(LD);   }
[lL][dD][iI]     { return(LDI);  }
[lL][dD][rR]     { return(LDR);  }
[lL][eE][aA]     { return(LEA);  }
[nN][oO][tT]     { return(NOT);  }
[rR][eE][tT]     { return(RET);  }
[rR][tT][iI]     { return(RTI);  }
[sS][tT]         { return(ST);   }
[sS][tT][iI]     { return(STI);  }
[sS][tT][rR]     { return(STR);  }
[tT][rR][aA][pP] { return(TRAP); }

[rR][0-7]        { return(REG);  }

\.[oO][rR][iI][gG] { return(ORIG);    }
\.[eE][nN][dD]     { return(END);     }
\.[sS][tT][rR][iI][nN][gG][zZ] { return(STRINGZ); }

0?[xX][0-9a-fA-F]{1,4} {
  // TODO there should be a better way
  char *p = yytext;
  while(*p == 'x' || *p == 'X')
    p++;
  long result = strtol(p, 0, 16);
  yylval.addr = result;
  return(HEXLIT);
}

[_a-zA-Z][_\-a-zA-Z0-9]* {
  return(IDENT);
}

[ \t\n]* // ignore whitespace
