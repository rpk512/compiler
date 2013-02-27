%{

#include "AST.h"

extern ModuleNode* module;

extern int yylex(void);

int yyerror(const char*);

#define YYDEBUG 1
#define YYERROR_VERBOSE

%}

%union {
    ModuleNode*    module;
    FunctionNode*  function;
    FunctionCall*  function_call;
    Block*         block;
    Statement*     statement;
    If*            else_chain;
    Expression*    expression;
    Symbol*        symbol;
    SourceText*    source_text;
    SourceLocation location;
    vector<unique_ptr<Declaration>>* argument_list;
    vector<unique_ptr<Statement>>*   statement_list;
    vector<unique_ptr<Expression>>*  expr_list;
}

%type <module> module
%type <function> function
%type <function_call> function_call
%type <block> block
%type <statement> statement
%type <else_chain> else_chain
%type <expression> expr
%type <expression> expr2
%type <argument_list> argument_list
%type <argument_list> actual_argument_list
%type <statement_list> statement_list
%type <expr_list> expr_list

%token<location> WHILE
%token<location> IF
%token<location> ELIF
%token<location> ELSE
%token<location> EXTERN

%token<location> EQUAL
%token<location> NOT_EQUAL
%token<location> LE
%token<location> GE
%token<location> LOGICAL_OR
%token<location> LOGICAL_AND

%token<location> RETURN

%token<location> TRUE
%token<location> FALSE

%token <symbol> ID
%token <source_text> NUMBER
%token <source_text> STRING_CONSTANT

%left LOGICAL_OR
%left LOGICAL_AND
%left EQUAL NOT_EQUAL
%left '<' '>' LE GE
%left '+' '-'
%left '/' '*' '%'

%%

module:
    function
    {
        module = new ModuleNode();
        module->functions.push_back(unique_ptr<FunctionNode>($1));
        $$ = module;
    }
|   module function
    {
        $1->functions.push_back(unique_ptr<FunctionNode>($2));
    }
;

function:
    ID ID '(' argument_list ')' block
    {
        $$ = new FunctionNode();
        if ($4 != nullptr) {
            $$->arguments = move(*$4);
            delete $4;
        }
        $$->block.reset($6);
        $$->id = *$2;
        delete $2;
        $$->returnTypeSym = *$1;
        $$->location = $1->location;
        delete $1;
    }
|   EXTERN ID ID '(' argument_list ')' ';'
    {
        $$ = new FunctionNode();
        if ($5 != nullptr) {
            $$->arguments = move(*$5);
            delete $5;
        }
        $$->id = *$3;
        delete $3;
        $$->returnTypeSym = *$2;
        delete $2;
        $$->location = $1;
    }
;

argument_list:
    /* empty */
    {
        $$ = nullptr;
    }
|   actual_argument_list
    {
        $$ = $1;
    }
;

actual_argument_list:
    ID ID
    {
        unique_ptr<Declaration> decl(new Declaration(*$1, *$2, nullptr));
        delete $1;
        delete $2;

        $$ = new vector<unique_ptr<Declaration>>();
        $$->push_back(move(decl));
    }
|   argument_list ',' ID ID
    {
        unique_ptr<Declaration> decl(new Declaration(*$3, *$4, nullptr));
        delete $3;
        delete $4;

        $1->push_back(move(decl));
        $$ = $1;
    }
;

block:
    '{' statement_list '}'
    {
        $$ = new Block();
        $$->statements = move(*$2);
        // TODO: location
        delete $2;
    }
;

statement_list:
    /* empty */
    {
        $$ = new vector<unique_ptr<Statement>>();
    }
|   statement_list statement
    {
        $1->push_back(unique_ptr<Statement>($2));
        $$ = $1;
    }
;

statement:
    ID ID ';'
    {
        $$ = new Declaration(*$1, *$2, nullptr);
        $$->location = $1->location;
        delete $1;
        delete $2;
    }
|   ID '=' expr ';'
    {
        Assignment* ass = new Assignment();
        ass->id = *$1;
        ass->rhs.reset($3);
        ass->location = $1->location;
        delete $1;

        $$ = ass;
    }
|   RETURN expr ';'
    {
        $$ = new Return($2);
        $$->location = $1;
    }
|   function_call ';'
    {
        $$ = $1;
    }
|   WHILE '(' expr ')' block
    {
        $$ = new While($3, $5);
        $$->location = $1;
    }
|   IF '(' expr ')' block else_chain
    {
        $$ = new If($3, $5, $6);
        $$->location = $1;
    }
;

else_chain:
    /* empty */
    {
        $$ = nullptr;
    }
|   ELIF '(' expr ')' block else_chain
    {
        $$ = new If($3, $5, $6);
        $$->location = $1;
    }
|   ELSE block
    {
        $$ = new If(nullptr, $2, NULL);
        $$->location = $1;
    }
;

function_call:
    ID '(' expr_list ')'
    {
        $$ = new FunctionCall();
        $$->id = *$1;
        $$->arguments = move(*$3);
        $$->Statement::location = $$->id.location;
        delete $1;
        delete $3;
    }
|   ID '(' ')'
    {
        $$ = new FunctionCall();
        $$->id = *$1;
        $$->arguments = vector<unique_ptr<Expression>>();
        $$->Statement::location = $$->id.location;
        delete $1;
    }
;

expr_list:
    expr
    {
        $$ = new vector<unique_ptr<Expression>>();
        $$->push_back(unique_ptr<Expression>($1));
    }
|   expr_list ',' expr
    {
        $1->push_back(unique_ptr<Expression>($3));
    }
;

expr:
    expr LOGICAL_OR expr
    {
        $$ = new BinaryOpExpression(OP_LOGICAL_OR, $1, $3);
    }
|   expr LOGICAL_AND expr
    {
        $$ = new BinaryOpExpression(OP_LOGICAL_AND, $1, $3);
    }
|   expr EQUAL expr
    {
        $$ = new BinaryOpExpression(OP_EQUAL, $1, $3);
    }
|   expr NOT_EQUAL expr
    {
        $$ = new BinaryOpExpression(OP_NOT_EQUAL, $1, $3);
    }
|   expr '>' expr
    {
        $$ = new BinaryOpExpression(OP_GREATER, $1, $3);
    }
|   expr GE expr
    {
        $$ = new BinaryOpExpression(OP_GREATER_EQ, $1, $3);
    }
|   expr '<' expr
    {
        $$ = new BinaryOpExpression(OP_LESS, $1, $3);
    }
|   expr LE expr
    {
        $$ = new BinaryOpExpression(OP_LESS_EQ, $1, $3);
    }
|   expr '+' expr
    {
        $$ = new BinaryOpExpression(OP_ADD, $1, $3);
    }
|   expr '-' expr
    {
        $$ = new BinaryOpExpression(OP_SUB, $1, $3);
    }
|   expr '/' expr
    {
        $$ = new BinaryOpExpression(OP_DIV, $1, $3);
    }
|   expr '*' expr
    {
        $$ = new BinaryOpExpression(OP_MUL, $1, $3);
    }
|   expr '%' expr
    {
        $$ = new BinaryOpExpression(OP_MOD, $1, $3);
    }
|   expr2
    {
        $$ = $1;
    }
;

expr2:
    '(' expr ')'
    {
        $$ = $2;
    }
|   function_call
    {
        $$ = $1;
    }
|   ID
    {
        $$ = new VariableExpression(*$1);
        $$->location = $1->location;
        delete $1;
    }
|   NUMBER
    {
        // TODO: check for out of range literals
        $$ = new NumericLiteral(atoi($1->str.c_str()));
        $$->location = $1->location;
        delete $1;
    }
|   STRING_CONSTANT
    {
        module->strings.push_back($1->str);
        $$ = new StringLiteral($1->str, module->strings.size()-1);
        $$->location = $1->location;
        delete $1;
    }
|   TRUE
    {
        $$ = new BooleanLiteral(true);
        $$->location = $1;
    }
|   FALSE
    {
        $$ = new BooleanLiteral(false);
        $$->location = $1;
    }
|   '!' expr2
    {
        $$ = new UnaryOpExpression(OP_UNARY_LOGICAL_NOT, $2);
        // TODO: location should be the location of ! not the expression
    }
|   '-' expr2
    {
        $$ = new UnaryOpExpression(OP_UNARY_MINUS, $2);
        // TODO: location should be the location of - not the expression
    }
;

%%

int yyerror(char const* str)
{
    printf("%s\n", str);
    exit(1);
}
