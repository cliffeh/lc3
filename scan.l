%{
#include "lc3as.h"
#include "parse.h"
%}

%option noyywrap

%%
 /* op codes */
[aA][dD][dD]       { return(ADD);  }
[aA][nN][dD]       { return(AND);  }
[bB][rR][nN]?[zZ]?[pP]?            {
                     return(BR);   }
[jJ][mM][pP]       { return(JMP);  }
[jJ][sS][rR]       { return(JSR);  }
[jJ][sS][rR][rR]   { return(JSRR); }
[lL][dD]           { return(LD);   }
[lL][dD][iI]       { return(LDI);  }
[lL][dD][rR]       { return(LDR);  }
[lL][eE][aA]       { return(LEA);  }
[nN][oO][tT]       { return(NOT);  }
[rR][eE][tT]       { return(RET);  }
[rR][tT][iI]       { return(RTI);  }
[sS][tT]           { return(ST);   }
[sS][tT][iI]       { return(STI);  }
[sS][tT][rR]       { return(STR);  }
[tT][rR][aA][pP]   { return(TRAP); }

 /* registers */
[rR][0-7]          { return(REG);  }

 /* assembler directives */
\.[oO][rR][iI][gG] { return(ORIG); }
\.[eE][nN][dD]     { return(END);  }
\.[sS][tT][rR][iI][nN][gG][zZ] {
  return(STRINGZ);
}

 /* literals */
[xX][0-9a-fA-F]{1,4} {
  return(HEXLIT);
}
#(0|[1-9][0-9]*) {
  return(DECLIT);
}
[_a-zA-Z][_\-a-zA-Z0-9]* {
  return(LABEL);
}

, { return(','); }

[ \t\n]* // ignore whitespace
;.* // ignore comments

. { printf("unexpected input: %s\n", yytext); }
