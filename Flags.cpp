#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <iostream>
#include "Flags.h"
using namespace std;

bool Flags::printAST = false;
bool Flags::debugParser = false;
bool Flags::eliminateTailCalls = false;
string Flags::inputFileName;
string Flags::libDir;

static string getAbsolutePath(const char* path)
{
    char dir[PATH_MAX];
    if (getcwd(dir, sizeof(dir)) == NULL) {
        perror("Failed to get working directory: ");
        exit(1);
    }

    string result = string(dir) + '/' + string(path);

    if (result[result.length()-1] != '/') {
        result += '/';
    }

    return result;
}

void parseFlags(int argc, char** argv)
{
    bool inputFileFound = false;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            string flag(argv[i]);
            if (flag == "--print-ast") {
                Flags::printAST = true;
            } else if (flag == "--debug-parser") {
                Flags::debugParser = true;
            } else if (flag == "--eliminate-tail-recursion") {
                Flags::eliminateTailCalls = true;
            } else if (i < argc-1 && flag == "--lib-dir") {
                Flags::libDir = getAbsolutePath(argv[i+1]);
                i++;
            } else {
                cerr << "Invalid flag: " << flag << endl;
                exit(1);
            }
        } else if (!inputFileFound) {
            Flags::inputFileName = string(argv[i]);
            inputFileFound = true;
        }
    }

    if (!inputFileFound) {
        cerr << "No input file specified." << endl;
        exit(1);
    }
}
