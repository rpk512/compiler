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
    unique_ptr<Type> type;

    Variable(unique_ptr<Type> type, int stackOffset)
    {
        this->type = move(type);
        this->stackOffset = stackOffset;
    }

    Variable() {}
};

class SymbolTable {
private:
    unordered_map<string,shared_ptr<FunctionNode>> functions;
    unordered_map<string,shared_ptr<Variable>> variables;
    unordered_map<string,BasicTypeId> basicTypeIds;
public:
    SymbolTable();

    shared_ptr<FunctionType> getFunction(string name) const;
    shared_ptr<Variable> getVariable(string name) const;
    BasicTypeId getBasicTypeId(string name) const;
    
    void setFunction(string name, shared_ptr<FunctionNode> ftype);
    void setVariable(string name, shared_ptr<Variable> variable);
    
    void clearVariables();
};

#endif
