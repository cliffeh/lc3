%{
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

\.ORIG           { return(ORIG);    }
\.END            { return(END);     }
\.STRINGZ        { return(STRINGZ); }

[_a-zA-Z][_\-a-zA-Z0-9]* {
  return(IDENT);
}

[ \t]* // ignore whitespace
