#ifndef TYPE_H
#define TYPE_H

#include <string>
#include <memory>
#include "Symbol.h"
#include "SymbolTable.h"
using namespace std;

enum BasicTypeId {
    T_UNKNOWN,
    T_INT8,
    T_INT16,
    T_INT32,
    T_INT64,
    T_U8,
    T_U16,
    T_U32,
    T_U64,
    T_BOOL,
    T_STRING,
    T_VOID
};

enum TypeForm {
    TF_BASIC,
    TF_POINTER,
    TF_ARRAY
};

string typeToString(BasicTypeId);

struct Type : public ASTNode {
    TypeForm form;
    int size;

    virtual bool validate(SymbolTable&, ErrorCollector&) = 0;
    virtual ~Type();
};

struct BasicType : public Type {
    BasicTypeId typeId;
    Symbol symbol;

    BasicType(Symbol symbol) {
        this->symbol = symbol;
        this->form = TF_BASIC;
    }
    BasicType(BasicTypeId typeId) {
        this->typeId = typeId;
    }
    bool validate(SymbolTable&, ErrorCollector&);
};

struct ArrayType : public Type {
    unique_ptr<Type> base;
    int elements;

    ArrayType(unique_ptr<Type> base, int elements) {
        this->elements = elements;
        this->base = move(base);
        this->form = TF_ARRAY;
        this->size = base->size * elements + 4;
    }
    bool validate(SymbolTable&, ErrorCollector&);
};

struct PointerType : public Type {
    unique_ptr<Type> base;

    PointerType(unique_ptr<Type> base) {
        this->base = move(base);
        this->form = TF_POINTER;
        this->size = 8;
    }
    bool validate(SymbolTable&, ErrorCollector&);
};

#endif
