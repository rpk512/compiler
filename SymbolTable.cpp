#include "SymbolTable.h"
#include "AST.h"

SymbolTable::SymbolTable()
{
    /*basicTypeIds["int8"]  = T_INT8;
    basicTypeIds["int16"] = T_INT16;
    basicTypeIds["int32"] = T_INT32;*/
    basicTypeIds["int64"] = T_INT64;
    basicTypeIds["int"] = T_INT64;
    
    /*basicTypeIds["u8"]  = T_U8;
    basicTypeIds["u16"] = T_U16;
    basicTypeIds["u32"] = T_U32;
    basicTypeIds["u64"] = T_U64;*/

    basicTypeIds["bool"] = T_BOOL;
    basicTypeIds["string"] = T_STRING;
    basicTypeIds["void"] = T_VOID;
}

shared_ptr<FunctionNode> SymbolTable::getFunction(Symbol symbol) const
{
    auto itr = functions.find(symbol.qualifiedString());
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

BasicTypeId SymbolTable::getBasicTypeId(string name) const
{
    auto itr = basicTypeIds.find(name);
    if (itr == basicTypeIds.end()) {
        return T_UNKNOWN;
    }
    return itr->second;
}

void SymbolTable::setFunction(Symbol symbol, shared_ptr<FunctionNode> ftype)
{
    functions[symbol.qualifiedString()] = ftype;
}

void SymbolTable::setVariable(string name, shared_ptr<Variable> variable)
{
    variables[name] = variable;
}

void SymbolTable::removeVariable(string name)
{
    variables.erase(name);
}

void SymbolTable::clearVariables()
{
    variables.clear();
}
