#include <iostream>
#include <fstream>
#include <memory>
#include <string>

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#include "AST.h"
#include "Arguments.h"

using namespace std;

string readFile(const string& fileName)
{
    ifstream input(fileName.c_str());

    if (!input.is_open()) {
        cout << "Failed to open file: " << fileName << endl;
        exit(1);
    }

    char* buffer;
    int size;

    input.seekg(0, ios::end);
    size = input.tellg();
    input.seekg(0, ios::beg);

    buffer = new char[size+1];

    input.read(buffer, size);
    input.close();

    buffer[size] = '\0';

    string sourceCode(buffer);

    delete[] buffer;

    return move(sourceCode);
}

void assembleAndLink()
{
    bool assembleFailed = false;
    pid_t pid;
    
    pid = fork();
    if (pid == 0) {
        execlp("nasm", "nasm", "-f", "elf64", "output.s", NULL);
        perror("Failed to execute nasm: ");
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        if (WEXITSTATUS(status) != 0) {
            cout << "Assemble failed." << endl;
            assembleFailed = true;
        }
    } else {
        perror("Failed to fork nasm: ");
        assembleFailed = true;
    }

    if (assembleFailed) {
        return;
    }
    
    pid = fork();
    if (pid == 0) {
        execlp("ld", "ld", "-o", "output", "output.o", NULL);
        perror("Failed to execute ld: ");
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        if (WEXITSTATUS(status) != 0) {
            cout << "Link failed." << endl;
        }
    } else {
        perror("Failed to fork ld: ");
    }
}

char** getLines(string sourceStr)
{
    char* source = new char[sourceStr.length()+1];
    char** lines;
    int lineCount;

    for (int i = 0; i < sourceStr.length(); i++) {
        source[i] = sourceStr[i];
        if (source[i] == '\n') {
            lineCount++;
        }
    }

    lines = new char*[lineCount+1];

    char* lineStart = source;
    int line = 0;

    for (int i = 0; i < sourceStr.length(); i++) {
        if (source[i] == '\n') {
            source[i] = '\0';
            lines[line] = lineStart;
            lineStart = &source[i+1];
            line++;
        }
    }

    lines[line] = lineStart;
    source[sourceStr.length()] = '\0';

    return lines;
}

int main(int argc, char** argv)
{
    Arguments args(argc, argv);

    if (args.getInputFiles().empty()) {
        cout << "No input files specified." << endl;
        return 1;
    }

    string sourceCode = readFile(args.getInputFiles()[0]);

    char** sourceLines = getLines(sourceCode);

    unique_ptr<ModuleNode> ast(parse(sourceCode, args.isFlagSet("debugParser")));

    if (args.isFlagSet("printAST")) {
        cout << ast->toString() << endl;
    }

    string errors = ast->validate(sourceLines);

    if (errors.length() == 0) {
        ofstream out("output.s", ios::trunc);
        if (out) {
            out << ast->cgen();
            out.close();
            assembleAndLink();
        } else {
            cout << "Failed to write output file." << endl;
        }
    } else {
        cout << errors;
        cout << "Compilation failed." << endl;
    }

    delete[] sourceLines[0];
    delete[] sourceLines;

    return 0;
}
