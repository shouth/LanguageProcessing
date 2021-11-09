#include "token-list.h"
#include "id-list.h"
#include "scan.h"

/* keyword list */
struct KEY key[KEYWORDSIZE] = {
    { "and",       TAND       },
    { "array",     TARRAY     },
    { "begin",     TBEGIN     },
    { "boolean",   TBOOLEAN   },
    { "break",     TBREAK     },
    { "call",      TCALL      },
    { "char",      TCHAR      },
    { "div",       TDIV       },
    { "do",        TDO        },
    { "else",      TELSE      },
    { "end",       TEND       },
    { "false",     TFALSE     },
    { "if",        TIF        },
    { "integer",   TINTEGER   },
    { "not",       TNOT       },
    { "of",        TOF        },
    { "or",        TOR        },
    { "procedure", TPROCEDURE },
    { "program",   TPROGRAM   },
    { "read",      TREAD      },
    { "readln",    TREADLN    },
    { "return",    TRETURN    },
    { "then",      TTHEN      },
    { "true",      TTRUE      },
    { "var",       TVAR       },
    { "while",     TWHILE     },
    { "write",     TWRITE     },
    { "writeln",   TWRITELN   }
};

/* Token counter */
int numtoken[NUMOFTOKEN + 1];

/* string of each token */
char *tokenstr[NUMOFTOKEN + 1] = {
    "",
    "NAME", "program", "var", "array", "of", "begin", "end", "if", "then",
    "else", "procedure", "return", "call", "while", "do", "not", "or",
    "div", "and", "char", "integer", "boolean", "readln", "writeln", "true",
    "false", "NUMBER", "STRING", "+", "-", "*", "=", "<>", "<", "<=", ">",
    ">=", "(", ")", "[", "]", ":=", ".", ",", ":", ";", "read", "write", "break"
};
