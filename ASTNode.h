#ifndef AST_NODE_H
#define AST_NODE_H

#include <sstream>
#include <vector>
#include "SourceLocation.h"

class SymbolTable;
class ErrorCollector;

class ASTNode {
public:
    SourceLocation location;

    virtual bool validate(SymbolTable&, ErrorCollector&) {return true;}
    virtual std::string toString() const = 0;
    virtual void cgen(std::ostringstream&) {}
    virtual ~ASTNode() {}
};

#endif
