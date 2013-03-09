#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <memory>
#include <string>
#include "Type.h"
#include "Symbol.h"
#include "SymbolTable.h"
using namespace std;

enum BinaryOperator {
    OP_LOGICAL_OR,
    OP_LOGICAL_AND,
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_GREATER,
    OP_GREATER_EQ,
    OP_LESS,
    OP_LESS_EQ,
    OP_ADD,
    OP_SUB,
    OP_DIV,
    OP_MUL,
    OP_MOD
};

enum UnaryOperator {
    OP_UNARY_LOGICAL_NOT,
    OP_UNARY_MINUS
};

string binaryOpToString(BinaryOperator);
string unaryOpToString(UnaryOperator);

class ErrorCollector;

class Expression {
public:
    SourceLocation location;
    shared_ptr<Type> type;
    int temporarySpace = 0;

    virtual ~Expression() {}
    virtual void cgen(ostringstream&) = 0;
    virtual void docgen(ostringstream& out) {cgen(out);}
    virtual bool validate(SymbolTable&, ErrorCollector&) = 0;
    virtual string toString() const = 0;
};

class BinaryOpExpression : public Expression {
public:
    BinaryOperator op;
    unique_ptr<Expression> lhs;
    unique_ptr<Expression> rhs;

    BinaryOpExpression(BinaryOperator op, Expression* lhs, Expression* rhs) {
        this->op = op;
        this->lhs.reset(lhs);
        this->rhs.reset(rhs);
        this->location = lhs->location;
        type.reset(new BasicType(T_UNKNOWN));
    }
    string toString() const;
    bool validate(SymbolTable&, ErrorCollector&);
    void cgen(ostringstream&);
    void docgen(ostringstream&);
};

class UnaryOpExpression : public Expression {
public:
    UnaryOperator op;
    unique_ptr<Expression> expr;

    UnaryOpExpression(UnaryOperator op, Expression* expr) {
        this->op = op;
        this->expr.reset(expr);
        this->location = expr->location;
        type.reset(new BasicType(T_UNKNOWN));
    }
    string toString() const;
    bool validate(SymbolTable&, ErrorCollector&);
    void cgen(ostringstream&);
};

class VariableExpression : public Expression {
public:
    Symbol id;
    shared_ptr<Variable> variable;

    VariableExpression(const Symbol& id) {
        this->id = id;
        type.reset(new BasicType(T_UNKNOWN));
    }
    string toString() const;
    bool validate(SymbolTable&, ErrorCollector&);
    void cgen(ostringstream&);
};

class Literal : public Expression {
public:
    bool validate(SymbolTable&, ErrorCollector&) {return true;};
};

class BooleanLiteral : public Literal {
public:
    bool value;

    BooleanLiteral(bool value) {
        this->value = value;
        type.reset(new BasicType(T_BOOL));
    }
    string toString() const;
    void cgen(ostringstream&);
};

class NumericLiteral : public Literal {
public:
    long value;

    NumericLiteral(long value) {
        this->value = value;
        type.reset(new BasicType(T_INT64));
    }
    string toString() const;
    void cgen(ostringstream&);
};

class StringLiteral : public Literal {
public:
    int poolIndex;
    string value;

    StringLiteral(const string& value, int poolIndex) {
        this->value = value;
        this->poolIndex = poolIndex;
        type.reset(new BasicType(T_STRING));
    }
    string toString() const;
    void cgen(ostringstream&);
};

#endif
