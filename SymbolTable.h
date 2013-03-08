#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include "Type.h"
using namespace std;

class SymbolTable;
class FunctionNode;

class Variable {
public:
    int stackOffset;
    Type type;

    Variable(Type type, int stackOffset)
    {
        this->type = type;
        this->stackOffset = stackOffset;
    }

    Variable() {}
};

class SymbolTable {
private:
    unordered_map<string,shared_ptr<FunctionType>> functions;
    unordered_map<string,shared_ptr<Variable>> variables;
    unordered_map<string,BasicTypeId> basicTypeIds;
public:
    SymbolTable();

    shared_ptr<FunctionType> getFunction(string name) const;
    shared_ptr<Variable> getVariable(string name) const;
    BasicTypeId getBasicTypeId(string name) const;
    
    void setFunction(string name, shared_ptr<FunctionType> ftype);
    void setVariable(string name, shared_ptr<Variable> variable);
    
    void clearVariables();
};

#endif
