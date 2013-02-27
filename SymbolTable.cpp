#include "SymbolTable.h"
#include "AST.h"

SymbolTable::SymbolTable()
{
    types["int8"]  = T_INT8;
    types["int16"] = T_INT16;
    types["int32"] = T_INT32;
    types["int64"] = T_INT64;
    types["int"] = T_INT64;
    
    types["u8"]  = T_U8;
    types["u16"] = T_U16;
    types["u32"] = T_U32;
    types["u64"] = T_U64;

    types["bool"] = T_BOOL;
    types["string"] = T_STRING;
    types["void"] = T_VOID;
}

shared_ptr<FunctionType> SymbolTable::getFunction(string name) const
{
    auto itr = functions.find(name);
    if (itr == functions.end()) {
        return nullptr;
    } else {
        return itr->second;
    }
}

shared_ptr<Variable> SymbolTable::getVariable(string name) const
{
    auto itr = variables.find(name);
    if (itr == variables.end()) {
        return nullptr;
    }
    return itr->second;
}

Type SymbolTable::getType(string name) const
{
    auto itr = types.find(name);
    if (itr == types.end()) {
        return T_UNKNOWN;
    }
    return itr->second;
}

void SymbolTable::setFunction(string name, shared_ptr<FunctionType> ftype)
{
    functions[name] = ftype;
}

void SymbolTable::setVariable(string name, shared_ptr<Variable> variable)
{
    variables[name] = variable;
}

void SymbolTable::clearVariables()
{
    variables.clear();
}

FunctionType::FunctionType(const unique_ptr<FunctionNode>& func,
                           const SymbolTable& symbols)
{
    returnType = symbols.getType(func->returnTypeSym.str);

    for (size_t i = 0; i < func->arguments.size(); i++) {
        Type type = symbols.getType(func->arguments[i]->typeSymbol.str);
        argumentTypes.push_back(type);
    }
}
