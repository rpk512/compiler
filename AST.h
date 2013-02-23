#ifndef AST_H
#define AST_H

#include <vector>
#include <string>
#include <sstream>
#include <memory>

#include "ASTNode.h"
#include "Expression.h"
#include "SymbolTable.h"
#include "Type.h"
#include "Symbol.h"

using namespace std;

class ModuleNode;
class FunctionNode;
class Block;
class Statement;
class Assignment;
class Declaration;
class Return;
class FunctionCall;
class If;
class While;

class ErrorCollector;

ModuleNode* parse(const string& codeString, bool debug);

class ModuleNode {
public:
    vector<unique_ptr<FunctionNode>> functions;

    string toString() const;
    string validate(char**);
    string cgen();
};

class FunctionNode : public ASTNode {
public:
    vector<unique_ptr<Declaration>> arguments;
    vector<shared_ptr<Variable>> locals;
    unique_ptr<Block> block;
    Symbol returnTypeSym;
    Symbol id;
    int stackSpaceForArgs = 0;

    string toString() const;
    bool validate(SymbolTable&, ErrorCollector&);
    void cgen(ostringstream&);
};

class Block : public ASTNode {
public:
    vector<unique_ptr<Statement>> statements;

    string toString() const {return toString(0);}
    string toString(int currentIndentLevel) const;
    bool validate(SymbolTable&, ErrorCollector&);
    void cgen(ostringstream&);
};

class Statement : public ASTNode {
public:
    string toString() const {return toString(0);}
    virtual string toString(int currentIndentLevel) const = 0;
    virtual ~Statement() {}
};

class Assignment : public Statement {
public:
    Symbol id;
    unique_ptr<Expression> rhs;
    shared_ptr<Variable> variable;
    
    string toString(int currentIndentLevel) const;
    bool validate(SymbolTable&, ErrorCollector&);
    void cgen(ostringstream&);
};

class Declaration : public Statement {
public:
    Symbol typeSymbol;
    Symbol id;
    unique_ptr<Expression> initialExpr;

    Declaration(const Symbol& typeSymbol,
                const Symbol& id,
                Expression* initialExpr) {
        this->typeSymbol = typeSymbol;
        this->id = id;
        this->initialExpr.reset(initialExpr);
        location = typeSymbol.location;
    }
    string toString() const;
    string toString(int currentIndentLevel) const;
    bool validate(SymbolTable&, ErrorCollector&);
};

class Return : public Statement {
public:
    unique_ptr<Expression> expr;

    Return(Expression* expr) {
        this->expr.reset(expr);
    }
    string toString(int currentIndentLevel) const;
    void cgen(ostringstream&);
};

class FunctionCall : public Statement, public Expression {
public:
    Symbol id;
    vector<unique_ptr<Expression>> arguments;
// TODO: add constructor
    string toString() const;
    string toString(int currentIndentLevel) const;
    bool validate(SymbolTable&, ErrorCollector&);
    Type getType(const SymbolTable&) const;
    void cgen(ostringstream&);
};

class If : public Statement {
public:
    unique_ptr<Block> block;
    unique_ptr<Expression> predicate;
    unique_ptr<If> elseClause;

    If(Expression* predicate, Block* block, If* elseClause) {
        this->block.reset(block);
        this->predicate.reset(predicate);
        this->elseClause.reset(elseClause);
    }
    string toString(int currentIndentLevel) const;
    bool validate(SymbolTable&, ErrorCollector&);
    void cgen(ostringstream&);
};

class While : public Statement {
public:
    unique_ptr<Expression> expr;
    unique_ptr<Block> block;

    While(Expression* expr, Block* block)   {
        this->expr.reset(expr);
        this->block.reset(block);
    }
    string toString(int currentIndentLevel) const;
    bool validate(SymbolTable&, ErrorCollector&);
    void cgen(ostringstream&);
};

#endif
