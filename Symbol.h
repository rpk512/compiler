#ifndef SYMBOL_H
#define SYMBOL_H

#include <string>
#include "SourceLocation.h"

class SourceText {
public:
    SourceLocation location;
    std::string str;
};

typedef SourceText Symbol;

#endif
