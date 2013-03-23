#include <stdlib.h>
#include <iostream>
#include "Flags.h"
using namespace std;

bool Flags::printAST = false;
bool Flags::debugParser = false;
bool Flags::eliminateTailCalls = false;
const char* Flags::inputFileName = nullptr;

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
            } else if (flag == "--eliminate-tail-calls") {
                Flags::eliminateTailCalls = true;
            }
        } else if (!inputFileFound) {
            Flags::inputFileName = argv[i];
            inputFileFound = true;
        }
    }

    if (!inputFileFound) {
        cout << "No input file specified." << endl;
        exit(1);
    }
}
