#ifndef SYMBOL_H
#define SYMBOL_H

#include <string>
#include "SourceLocation.h"

using namespace std;

struct SourceText {
    SourceLocation location;
    string str;
};

struct Symbol {
    SourceLocation location;
    string module;
    string str;

    string qualifiedString() const {
        return module + ":" + str;
    }

    string asmString() const {
        return "$" + module + "." + str;
    }
};

#endif
