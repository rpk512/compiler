#include <iostream>
#include <fstream>
#include <memory>
#include <string>

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#include "AST.h"
#include "Flags.h"
#include "cgen.h"

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
            exit(1);
        }
    } else {
        perror("Failed to fork nasm: ");
        assembleFailed = true;
        exit(1);
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
            exit(1);
        }
    } else {
        perror("Failed to fork ld: ");
        exit(1);
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

string getModuleName(string filePath)
{
    size_t slash = filePath.find_last_of('/');
    if (slash != string::npos) {
        filePath = filePath.substr(slash+1);
    }
    return filePath.substr(0, filePath.length()-2);
}

void compile(string filePath, ostream& assemblyOutput, SymbolTable& symbols)
{
    if (filePath.substr(filePath.length()-2) != ".u") {
        cout << "Source file '" << filePath;
        cout << "' does not have a .u extension" << endl;
        exit(1);
    }

    string sourceCode = readFile(filePath);

    unique_ptr<ModuleNode> ast(parse(sourceCode, getModuleName(filePath)));

    if (Flags::printAST) {
        cout << ast->toString() << endl;
    }

    for (Import& import : ast->imports) {
        assemblyOutput << ";; " << import.path.str << " ;;" << endl;
        if (import.isAssembly) {
            assemblyOutput << readFile(import.path.str+".s") << endl;
        } else {
            compile(import.path.str+".u", assemblyOutput, symbols);
        }
    }
    
    ast->sourceLines = getLines(sourceCode);

    string errors = ast->validate(symbols);

    if (errors.length() > 0) {
        cout << errors;
        cout << "Compilation failed." << endl;
        exit(1);
    }
    
    delete[] ast->sourceLines[0];
    delete[] ast->sourceLines;

    ast->cgen(assemblyOutput);
}

int main(int argc, char** argv)
{
    parseFlags(argc, argv);

    ofstream out("output.s", ios::trunc);
    if (!out) {
        cout << "Failed to open output.s" << endl;
        exit(1);
    }
    startAsm(out, getModuleName(Flags::inputFileName));

    SymbolTable symbols;

    compile(Flags::inputFileName, out, symbols);

    out.close();

    assembleAndLink();

    return 0;
}
