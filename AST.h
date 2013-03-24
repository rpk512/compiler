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

ModuleNode* parse(const string&, const string&);

struct Import {
    SourceText path;
    bool isAssembly;
};

struct ModuleNode {
    vector<shared_ptr<FunctionNode>> functions;
    vector<Import> imports;
    vector<string> strings;
    char** sourceLines;
    string name;

    string toString() const;
    string validate(SymbolTable&);
    void cgen(ostream&);
};

struct FunctionNode {
    SourceLocation location;
    vector<unique_ptr<Declaration>> arguments;
    vector<shared_ptr<Variable>> locals;
    unique_ptr<Block> block;
    shared_ptr<Type> returnType;
    Symbol id;
    int stackSpaceForArgs = 0;
    int stackSpaceForLocals = 0;
    int temporarySpace = 0;
    bool isTailRecursive = true;
    set<FunctionCall*> tailCalls;

    string toString() const;
    bool validateSignature(SymbolTable&, ErrorCollector&);
    bool validateBody(SymbolTable&, ErrorCollector&);
    void cgen(ostream&);
};

struct Block {
    SourceLocation location;
    vector<unique_ptr<Statement>> statements;

    string toString(int currentIndentLevel) const;
    bool validate(SymbolTable&, ErrorCollector&);
    void cgen(ostream&);
};

struct Statement {
    SourceLocation location;

    string toString() const {return toString(0);}
    virtual string toString(int currentIndentLevel) const = 0;
    virtual void cgen(ostream&) {};
    virtual bool validate(SymbolTable&, ErrorCollector&) = 0;
    virtual ~Statement() {}
};

// TODO: this should really reuse code from BinaryOpExpression
struct Assignment : public Statement {
    unique_ptr<Expression> rhs;
    unique_ptr<Expression> lhs;
    
    string toString(int currentIndentLevel) const;
    bool validate(SymbolTable&, ErrorCollector&);
    void cgen(ostream&);
};

struct Declaration : public Statement {
    shared_ptr<Type> type;
    vector<Symbol> ids;
    bool inOuterBlock = false;

    Declaration(Type* type,
                const vector<Symbol>& ids) {
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
    void cgen(ostream&);
};

struct FunctionCall : public Statement, public Expression {
    Symbol id;
    shared_ptr<FunctionNode> function;
    vector<unique_ptr<Expression>> arguments;

    string toString() const;
    string toString(int currentIndentLevel) const;
    bool validate(SymbolTable&, ErrorCollector&);
    void cgen(ostream&);
    void cgen(ostream&, bool);
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
    void cgen(ostream&);
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
    void cgen(ostream&);
};

struct RangeFor : public Statement {
    unique_ptr<Declaration> decl;
    shared_ptr<Variable> var;
    unique_ptr<Expression> start;
    unique_ptr<Expression> end;
    unique_ptr<Block> block;

    RangeFor(Type* type, const Symbol& id, Expression* start,
             Expression* end, Block* block) {
        vector<Symbol> ids;
        ids.push_back(id);
        this->decl.reset(new Declaration(type, move(ids)));
        this->start.reset(start);
        this->end.reset(end);
        this->block.reset(block);
    }
    string toString(int currentIndentLevel) const;
    bool validate(SymbolTable&, ErrorCollector&);
    void cgen(ostream&);
};

struct ArrayFor : public Statement {
    unique_ptr<Declaration> decl;
    shared_ptr<Variable> var;
    unique_ptr<Expression> arrayExpr;
    unique_ptr<Block> block;

    ArrayFor(Type* type, const Symbol& id, Expression* arrayExpr, Block* block) {
        vector<Symbol> ids;
        ids.push_back(id);
        this->decl.reset(new Declaration(type, move(ids)));
        this->arrayExpr.reset(arrayExpr);
        this->block.reset(block);
    }
    string toString(int currentIdentLevel) const;
    bool validate(SymbolTable&, ErrorCollector&);
    void cgen(ostream&);
};

#endif
