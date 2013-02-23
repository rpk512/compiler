%{
#include <string>
#include <iostream>

#include <stdio.h>

#include "AST.h"
#include "parser.tab.hh"


// After each rule, update the current source location
#define YY_BREAK {currentLocation.column += yyleng; break;}

#define RET(tok) {yylval.location = currentLocation; return tok;}

SourceLocation currentLocation;

static void nextLine()
{
    currentLocation.line++;
    currentLocation.column = 0;
}

%}

DIGIT [0-9]
ID    [a-zA-Z_][a-zA-Z_0-9]*

%%

True  RET(TRUE)
False RET(FALSE)

{DIGIT}+ {
    // it's lame to use Symbol here, but it has the data items we need
    yylval.symbol = new Symbol{currentLocation, string(yytext)};
    return NUMBER;
}

[{},();=><+-/*%!] { return yytext[0]; }

if       RET(IF)
elif     RET(ELIF)
else     RET(ELSE)
while    RET(WHILE)
return   RET(RETURN)
extern   RET(EXTERN)

"==" RET(EQUAL)
"!=" RET(NOT_EQUAL)
"<=" RET(LE)
">=" RET(GE)
"||" RET(LOGICAL_OR)
"&&" RET(LOGICAL_AND)

{ID} {
    yylval.symbol = new Symbol{currentLocation, string(yytext)};
    return ID;
}

[ \t]+ /* ignore */ ;

\n { nextLine(); }

#.*\n { nextLine(); }

. { cout << "Invalid character: " << yytext[0] << endl; exit(1); }

%%

extern int yydebug;

ModuleNode* module;

ModuleNode* parse(const string& codeString, bool debug)
{
    yy_scan_string(codeString.c_str());

    if (debug) {
        yydebug = 1;
    } else {
        yydebug = 0;
    }

    currentLocation.line = 0;
    currentLocation.column = 1;

    yyparse();

    yylex_destroy();

    return module;
}

