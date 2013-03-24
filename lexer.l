%{
#include <string>
#include <iostream>

#include <stdio.h>

#include "AST.h"
#include "Flags.h"
#include "parser.tab.hh"


// After each rule, update the current source location
#define YY_BREAK {currentLocation.column += yyleng; break;}

#define RET(tok) {yylval.location = currentLocation; return tok;}

SourceLocation currentLocation;

string currentModuleName;

static void nextLine()
{
    currentLocation.line++;
    currentLocation.column = -yyleng;
}

%}

DIGIT [0-9]
ID    [a-zA-Z_][a-zA-Z_0-9]*

%%

["].*["] {
    string str(yytext);
    str = str.substr(1, str.length()-2);
    yylval.source_text = new SourceText{currentLocation, str};
    return STRING_CONSTANT;
}

True  RET(TRUE)
False RET(FALSE)

{DIGIT}+ {
    yylval.source_text = new SourceText{currentLocation, string(yytext)};
    return NUMBER;
}

[{},();=><+-/*%!\[\]&] { return yytext[0]; }

if       RET(IF)
elif     RET(ELIF)
else     RET(ELSE)
while    RET(WHILE)
return   RET(RETURN)
extern   RET(EXTERN)
var      RET(VAR)
import   RET(IMPORT)
asm      RET(ASM)

"==" RET(EQUAL)
"!=" RET(NOT_EQUAL)
"<=" RET(LE)
">=" RET(GE)
"||" RET(LOGICAL_OR)
"&&" RET(LOGICAL_AND)

{ID}:{ID} {
    string s(yytext);
    size_t loc = s.find(':');

    yylval.symbol = new Symbol{currentLocation,
                               s.substr(0, loc),
                               s.substr(loc+1)};
    return QUALIFIED_ID;
}

{ID} {
    yylval.symbol = new Symbol{currentLocation,
                               currentModuleName,
                               string(yytext)};
    return ID;
}

[ \t]+ /* ignore */ ;

\n { nextLine(); }

#.*\n { nextLine(); }

. {
    cerr << "Invalid character '" << yytext[0] << "' found at ";
    cerr << currentLocation.line+1 << ":" << currentLocation.column << endl;
    exit(1);
}

%%

extern int yydebug;
extern int yyparse();

ModuleNode* module;

ModuleNode* parse(const string& codeString,
                  const string& moduleName)
{
    yy_scan_string(codeString.c_str());

    if (Flags::debugParser) {
        yydebug = 1;
    } else {
        yydebug = 0;
    }

    currentLocation.line = 0;
    currentLocation.column = 1;
    currentModuleName = moduleName;

    module = new ModuleNode();
    module->name = moduleName;

    yyparse();

    yylex_destroy();

    return module;
}

