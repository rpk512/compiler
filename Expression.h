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
    OP_MOD,
    OP_ARRAY_ACCESS
};

enum UnaryOperator {
    OP_LOGICAL_NOT,
    OP_UNARY_MINUS,
    OP_ADDRESS,
    OP_DEREF
};

string binaryOpToString(BinaryOperator);
string unaryOpToString(UnaryOperator);

class ErrorCollector;

struct Expression {
    SourceLocation location;
    shared_ptr<Type> type;
    int temporarySpace = 0;

    virtual ~Expression() {}
    virtual void cgen(ostream&, bool genAddress=false) = 0;
    virtual void docgen(ostream& out, bool genAddress=false) {
        cgen(out, genAddress);
    }
    virtual bool validate(SymbolTable&, ErrorCollector&) = 0;
    virtual string toString() const = 0;
    virtual bool isAddressable() const = 0;
};

struct BinaryOpExpression : public Expression {
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
    void cgen(ostream&, bool);
    void docgen(ostream&, bool);
    bool isAddressable() const {return op == OP_ARRAY_ACCESS;}
};

struct UnaryOpExpression : public Expression {
    UnaryOperator op;
    unique_ptr<Expression> expr;

    UnaryOpExpression(UnaryOperator op, Expression* expr) {
        this->op = op;
        this->expr.reset(expr);
        this->location = expr->location;
        type.reset(new BasicType(T_UNKNOWN));
    }
    UnaryOpExpression(UnaryOperator op, unique_ptr<Expression>& expr) {
        this->op = op;
        this->expr = move(expr);
        this->location = this->expr->location;
        type.reset(new BasicType(T_UNKNOWN));
    }
    string toString() const;
    bool validate(SymbolTable&, ErrorCollector&);
    void cgen(ostream&, bool);
    bool isAddressable() const {return op == OP_DEREF;}
};

struct VariableExpression : public Expression {
    Symbol id;
    shared_ptr<Variable> variable;

    VariableExpression(const Symbol& id) {
        this->id = id;
        type.reset(new BasicType(T_UNKNOWN));
    }
    string toString() const;
    bool validate(SymbolTable&, ErrorCollector&);
    void cgen(ostream&, bool);
    bool isAddressable() const {return true;}
};

struct Literal : public Expression {
    bool validate(SymbolTable&, ErrorCollector&) {return true;};
    bool isAddressable() const {return false;}
};

struct BooleanLiteral : public Literal {
    bool value;

    BooleanLiteral(bool value) {
        this->value = value;
        type.reset(new BasicType(T_BOOL));
    }
    string toString() const;
    void cgen(ostream&, bool);
};

struct NumericLiteral : public Literal {
    long value;

    NumericLiteral(long value) {
        this->value = value;
        type.reset(new BasicType(T_INT64));
    }
    string toString() const;
    void cgen(ostream&, bool);
};

struct StringLiteral : public Literal {
    int poolIndex;
    string value;

    StringLiteral(const string& value, int poolIndex) {
        this->value = value;
        this->poolIndex = poolIndex;
        type.reset(new BasicType(T_STRING));
    }
    string toString() const;
    void cgen(ostream&, bool);
};

#endif
