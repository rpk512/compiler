#ifndef FLAGS_H
#define FLAGS_H

#include <string>

struct Flags {
    static bool debugParser;
    static bool printAST;
    static bool eliminateTailCalls;
    static std::string inputFileName;
    static std::string libDir;
};

extern void parseFlags(int, char**);

#endif
