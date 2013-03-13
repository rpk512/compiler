#ifndef AST_H
#define AST_H

#include <vector>
#include <set>
#include <string>
#include <sstream>
#include <memory>

#include "Expression.h"
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
class SymbolTable;

ModuleNode* parse(const string& codeString, bool debug);

struct ModuleNode {
    vector<shared_ptr<FunctionNode>> functions;
    vector<string> strings;

    string toString() const;
    string validate(char**);
    string cgen(bool);
};

struct FunctionNode {
    SourceLocation location;
    vector<unique_ptr<Declaration>> arguments;
    vector<shared_ptr<Variable>> locals;
    unique_ptr<Block> block;
    shared_ptr<Type> returnType;
    Symbol id;
    int stackSpaceForArgs = 0;
    int temporarySpace = 0;
    bool isTailRecursive = true;
    set<FunctionCall*> tailCalls;

    string toString() const;
    bool validateSignature(SymbolTable&, ErrorCollector&);
    bool validateBody(SymbolTable&, ErrorCollector&);
    void cgen(ostringstream&);
};

struct Block {
    SourceLocation location;
    vector<unique_ptr<Statement>> statements;

    string toString(int currentIndentLevel) const;
    bool validate(SymbolTable&, ErrorCollector&);
    void cgen(ostringstream&);
};

struct Statement {
    SourceLocation location;

    string toString() const {return toString(0);}
    virtual string toString(int currentIndentLevel) const = 0;
    virtual void cgen(ostringstream&) {};
    virtual bool validate(SymbolTable&, ErrorCollector&) = 0;
    virtual ~Statement() {}
};

// TODO: this should really reuse code from BinaryOpExpression
struct Assignment : public Statement {
    unique_ptr<Expression> rhs;
    unique_ptr<Expression> lhs;
    
    string toString(int currentIndentLevel) const;
    bool validate(SymbolTable&, ErrorCollector&);
    void cgen(ostringstream&);
};

struct Declaration : public Statement {
    shared_ptr<Type> type;
    vector<Symbol> ids;

    Declaration(Type* type,
                const vector<Symbol>& ids,
                Expression* initialExpr) {
        this->type.reset(type);
        this->ids = ids;
        location = type->location;
    }
    string toString() const;
    string toString(int currentIndentLevel) const;
    bool validate(SymbolTable&, ErrorCollector&);
};

struct Return : public Statement {
    unique_ptr<Expression> expr;

    Return(Expression* expr) {
        this->expr.reset(expr);
    }
    string toString(int currentIndentLevel) const;
    bool validate(SymbolTable&, ErrorCollector&);
    void cgen(ostringstream&);
};

struct FunctionCall : public Statement, public Expression {
    Symbol id;
    shared_ptr<FunctionNode> function;
    vector<unique_ptr<Expression>> arguments;

// TODO: add constructor
    string toString() const;
    string toString(int currentIndentLevel) const;
    bool validate(SymbolTable&, ErrorCollector&);
    void cgen(ostringstream&);
    void cgen(ostringstream&, bool);
    bool isAddressable() const {return false;}
};

struct If : public Statement {
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

struct While : public Statement {
    unique_ptr<Expression> expr;
    unique_ptr<Block> block;

    While(Expression* expr, Block* block) {
        this->expr.reset(expr);
        this->block.reset(block);
    }
    string toString(int currentIndentLevel) const;
    bool validate(SymbolTable&, ErrorCollector&);
    void cgen(ostringstream&);
};

#endif
