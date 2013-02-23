#include <iostream>
#include <stdio.h>
#include "Arguments.h"
using namespace std;

Arguments::Arguments(int argc, char** argv)
{
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            string flagName(argv[i]);
            flags.insert(flagName.substr(1));
        } else {
            inputFileNames.push_back(argv[i]);
        }
    }
}

bool Arguments::isFlagSet(const string& flagName) const
{
    return flags.find(flagName) != flags.end();
}

vector<string> Arguments::getInputFiles() const
{
    return inputFileNames;
}
