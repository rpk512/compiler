#ifndef TYPE_H
#define TYPE_H

#include <string>
#include <memory>
#include "Symbol.h"
using namespace std;

class SymbolTable;
class ErrorCollector;

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

struct Type {
    SourceLocation location;
    TypeForm form;
    int size;

    virtual ~Type() {};
    virtual bool validate(SymbolTable&, ErrorCollector&) = 0;
    virtual string toString() const = 0;
    virtual bool isVoid() const {return false;}
    virtual bool isBasicType(BasicTypeId) const {return false;}
    virtual bool isCompatible(Type*) const {return false;}
};

struct BasicType : public Type {
    BasicTypeId typeId;
    Symbol symbol;

    BasicType(Symbol symbol) {
        this->symbol = symbol;
        this->location = symbol.location;
        this->typeId = T_UNKNOWN;
        this->form = TF_BASIC;
        this->size = 8;
    }
    BasicType(BasicTypeId typeId) {
        this->typeId = typeId;
        this->form = TF_BASIC;
        this->size = 8;
        this->symbol.str = typeToString(typeId);
    }
    bool validate(SymbolTable&, ErrorCollector&);
    string toString() const;
    bool isCompatible(Type*) const;
    bool isVoid() const {return typeId == T_VOID;}
    bool isBasicType(BasicTypeId id) const {return id == typeId;}
};

struct ArrayType : public Type {
    shared_ptr<Type> base;
    int elements;

    ArrayType(shared_ptr<Type> base, int elements) {
        this->elements = elements;
        this->form = TF_ARRAY;
        this->size = base->size * elements;
        this->location = base->location;
        this->base = base;
    }
    bool validate(SymbolTable&, ErrorCollector&);
    string toString() const;
    bool isCompatible(Type*) const;
};

struct PointerType : public Type {
    shared_ptr<Type> base;

    PointerType(shared_ptr<Type> base) {
        this->form = TF_POINTER;
        this->size = 8;
        this->location = base->location;
        this->base = base;
    }
    bool validate(SymbolTable&, ErrorCollector&);
    string toString() const;
    bool isCompatible(Type*) const;
};

#endif
