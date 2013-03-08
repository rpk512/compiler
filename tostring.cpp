#include <sstream>
#include <iostream>
#include "AST.h"
using namespace std;

string binaryOpToString(BinaryOperator op) {
    switch (op) {
        case OP_LOGICAL_OR:  return "||";
        case OP_LOGICAL_AND: return "&&";
        case OP_EQUAL:       return "==";
        case OP_NOT_EQUAL:   return "!=";
        case OP_GREATER:     return ">";
        case OP_GREATER_EQ:  return ">=";
        case OP_LESS:        return "<";
        case OP_LESS_EQ:     return "<=";
        case OP_ADD:         return "+";
        case OP_SUB:         return "-";
        case OP_DIV:         return "/";
        case OP_MUL:         return "*";
        case OP_MOD:         return "%";
    }
}

string unaryOpToString(UnaryOperator op) {
    switch (op) {
        case OP_UNARY_LOGICAL_NOT: return "!";
        case OP_UNARY_MINUS:       return "-";
    }
}

string typeToString(Type type) {
    switch (type) {
        case T_INT8:    return "int8";
        case T_INT16:   return "int16";
        case T_INT32:   return "int32";
        case T_INT64:   return "int64";
        case T_U8:      return "u8";
        case T_U16:     return "u16";
        case T_U32:     return "u32";
        case T_U64:     return "u64";
        case T_BOOL:    return "bool";
        case T_VOID:    return "void";
        case T_UNKNOWN: return "#UNKNOWN_TYPE#";
    }
}

static string indent(int size) {
    string s = "";
    for (int i = 0; i < size; i++) {
        s += "    ";
    }
    return s;
}


string ModuleNode::toString() const {
    string s = "";
    for (size_t i = 0; i < functions.size(); i++) {
        s += functions[i]->toString() + "\n\n";
    }
    return s;
}


string FunctionNode::toString() const {
    string s = "";
    if (block == nullptr) {
        s += "extern ";
    }
    s += returnTypeSym.str + " " + id.str + "(";
    for (size_t i = 0; i < arguments.size(); i++) {
        s += arguments[i]->typeSymbol.str + " ";
        s += arguments[i]->id.str;
        if (i != arguments.size() - 1) {
            s += ", ";
        }
    }
    s += ")";
    if (block == nullptr) {
        s += ";";
    } else {
        s += " " + block->toString(0);
    }
    return s;
}


string Block::toString(int currentIndentLevel) const {
    string s = "{\n";
    for (size_t i = 0; i < statements.size(); i++) {
        s += indent(currentIndentLevel+1);
        s += statements[i]->toString(currentIndentLevel+1) + "\n";
    }
    s += indent(currentIndentLevel) + "}";
    return s;
}


string Assignment::toString(int currentIndentLevel) const {
    return id.str + " = " + rhs->toString() + ";";
}


string Declaration::toString() const {
    return typeSymbol.str + " " + id.str;
}

string Declaration::toString(int currentIndentLevel) const {
    return toString() + ";";
}


string Return::toString(int currentIndentLevel) const {
    return "return " + expr->toString() + ";";
}


string FunctionCall::toString() const {
    string s = id.str + "(";
    for (size_t i = 0; i < arguments.size(); i++) {
        s += arguments[i]->toString();
        if (i != arguments.size()-1) {
            s += ", ";
        }
    }
    s += ")";
    return s;
}

string FunctionCall::toString(int currentIndentLevel) const {
    return toString() + ";";
}


string If::toString(int currentIndentLevel) const {
    string s = "";
    if (predicate != nullptr) {
        s += "if (" + predicate->toString() + ") ";
    } else {
        s += "se ";
    }
    s += block->toString(currentIndentLevel);
    if (elseClause != nullptr) {
        s += " el" + elseClause->toString(currentIndentLevel);
    }
    return s;
}


string While::toString(int currentIndentLevel) const {
    string s = "while (" + expr->toString() + ") ";
    s += block->toString(currentIndentLevel);
    return s;
}




string BinaryOpExpression::toString() const {
    return "(" + lhs->toString() + " " +
           binaryOpToString(op) + " " +
           rhs->toString() + ")";
}


string UnaryOpExpression::toString() const {
    return unaryOpToString(op) + expr->toString();
}


string BooleanLiteral::toString() const {
    return value ? "True" : "False";
}


string NumericLiteral::toString() const {
    return to_string(value);
}


string VariableExpression::toString() const {
    return id.str;
}

string StringLiteral::toString() const {
    return '"' + value + '"';
}


string BasicType::toString() const {
    return typeToString(typeId);
}

string ArrayType::toString() const {
    return base->toString() + "[" + to_string(elements) + "]";
}

string PointerType::toString() const {
    return base->toString() + "*";
}
