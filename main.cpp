#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <set>

#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>

#include "AST.h"
#include "Flags.h"
#include "cgen.h"

using namespace std;

string readFile(const string& workingDir, const string& fileName)
{
    ifstream input((workingDir + fileName).c_str());

    if (!input.is_open()) {
        cerr << "Failed to find file: " << fileName;
        cerr << " in " << workingDir << endl;
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
            cerr << "Assemble failed." << endl;
            exit(1);
        }
    } else {
        perror("Failed to fork nasm: ");
        exit(1);
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
            cerr << "Link failed." << endl;
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
    int lineCount = 0;

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
    return filePath.substr(0, filePath.length());
}

string getModuleDir(const string& workingDir, const string& filePath)
{
    string basePath;

    if (access((workingDir + filePath).c_str(), F_OK) != -1) {
        basePath = workingDir + filePath;
    } else {
        if (access((Flags::libDir + filePath).c_str(), F_OK) == -1) {
            cerr << "Failed to locate module: " << filePath << endl;
            cerr << "Working directory: " << workingDir << endl;
            cerr << "Library directory: " << Flags::libDir << endl;
            exit(1);
        } else {
            basePath = Flags::libDir + filePath;
        }
    }

    size_t slash = basePath.find_last_of('/');

    if (slash != string::npos) {
        basePath = basePath.substr(0, slash+1);
    } else {
        basePath = "./";
    }

    return basePath;
}

string getWorkingDir()
{
    char dir[PATH_MAX];
    if (getcwd(dir, sizeof(dir)) == NULL) {
        perror("Failed to get working directory: ");
        exit(1);
    }
    return string(dir) + '/';
}

void compile(string workingDir, string moduleName,
             ostream& assemblyOutput, SymbolTable& symbols)
{
    static set<string> importedModules;

    string sourceCode = readFile(workingDir, moduleName+".u");

    unique_ptr<ModuleNode> ast(parse(sourceCode, moduleName));

    if (Flags::printAST) {
        cout << ast->toString() << endl;
    }

    for (Import& import : ast->imports) {
        if (import.isAssembly) {
            string importPath = workingDir+import.path.str+".s";
            if (importedModules.find(importPath) != importedModules.end()) {
                continue;
            }
            importedModules.insert(importPath);
            assemblyOutput << ";; " << import.path.str << " ;;" << endl;

            assemblyOutput << readFile(workingDir, import.path.str+".s") << endl;
        } else {
            string newModuleName = getModuleName(import.path.str);
            string newWorkingDir = getModuleDir(workingDir, import.path.str+".u");

            string importPath = newWorkingDir + newModuleName;
            if (importedModules.find(importPath) != importedModules.end()) {
                continue;
            }
            importedModules.insert(importPath);
            assemblyOutput << ";; " << import.path.str << " ;;" << endl;

            compile(newWorkingDir, newModuleName, assemblyOutput, symbols);
        }
    }
    
    ast->sourceLines = getLines(sourceCode);

    string errors = ast->validate(symbols);

    if (errors.length() > 0) {
        cerr << errors;
        cerr << "Compilation failed." << endl;
        exit(1);
    }
    
    delete[] ast->sourceLines[0];
    delete[] ast->sourceLines;

    ast->cgen(assemblyOutput);
}

int main(int argc, char** argv)
{
    parseFlags(argc, argv);

    string moduleName = getModuleName(Flags::inputFileName);
    moduleName = moduleName.substr(0, moduleName.length()-2);

    ofstream out("output.s", ios::trunc);
    if (!out) {
        cerr << "Failed to open output.s" << endl;
        exit(1);
    }

    startAsm(out, moduleName);

    SymbolTable symbols;

    string dir = getModuleDir(getWorkingDir(), Flags::inputFileName);
    compile(dir, moduleName, out, symbols);

    out.close();

    assembleAndLink();

    return 0;
}
