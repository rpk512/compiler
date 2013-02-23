#ifndef TYPE_H
#define TYPE_H

#include <string>

enum Type {
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
    T_VOID
};

std::string typeToString(Type);

#endif
