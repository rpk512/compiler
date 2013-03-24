#include "ErrorCollector.h"
using namespace std;

ErrorCollector::ErrorCollector(char** sourceLines, const string& fileName)
{
    this->sourceLines = sourceLines;
    this->fileName = fileName;
}

string ErrorCollector::getErrorString() const
{
    return errors.str();
}

void ErrorCollector::putLoc(SourceLocation location)
{
    errors << fileName << " ";
    errors << location.line+1 << ":" << location.column << " ";
}

void ErrorCollector::putLine(SourceLocation location)
{
    errors << "    " << sourceLines[location.line] << "\n\n";
}

void ErrorCollector::error(SourceLocation location, string message)
{
    putLoc(location);
    errors << message << "\n";
    putLine(location);
}

void ErrorCollector::undefinedVariable(SourceLocation location, string id)
{
    putLoc(location);
    errors << "Reference to undefined variable: " << id << "\n";
    putLine(location);
}

void ErrorCollector::undefinedFunction(SourceLocation location, string id)
{
    putLoc(location);
    errors << "Reference to undefined function: " << id << "\n";
    putLine(location);
}

void ErrorCollector::unexpectedType(SourceLocation location,
                                    Type* expected, Type* found)
{
    putLoc(location);
    errors << "Expected expression of type '" << expected->toString();
    errors << "', found '" << found->toString() << "' \n";
    putLine(location);
}
