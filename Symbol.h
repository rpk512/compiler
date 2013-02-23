#ifndef SYMBOL_H
#define SYMBOL_H

#include <string>
#include "SourceLocation.h"

class Symbol {
public:
    SourceLocation location;
    std::string str;
};

#endif
