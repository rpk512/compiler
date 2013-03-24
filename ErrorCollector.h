#ifndef ERROR_COLLECTOR
#define ERROR_COLLECTOR

#include <string>
#include <sstream>
#include "SourceLocation.h"
#include "Type.h"
using namespace std;

class ErrorCollector {
private:
    ostringstream errors;
    char** sourceLines;
    string fileName;
    void putLoc(SourceLocation);
    void putLine(SourceLocation);
public:
    ErrorCollector(char** sourceLines, const string& fileName);
    void error(SourceLocation, string);
    void undefinedVariable(SourceLocation, string);
    void undefinedFunction(SourceLocation, string);
    void unexpectedType(SourceLocation, Type*, Type*);
    string getErrorString() const;
};

#endif
