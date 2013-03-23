#ifndef FLAGS_H
#define FLAGS_H

#include <string>

struct Flags {
    static bool debugParser;
    static bool printAST;
    static bool eliminateTailCalls;
    static const char* inputFileName;
};

extern void parseFlags(int, char**);

#endif
