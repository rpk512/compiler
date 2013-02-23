#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include <vector>
#include <set>
#include <string>

class Arguments {
private:
    std::vector<std::string> inputFileNames;
    std::set<std::string> flags;
public:
    Arguments(int argc, char** argv);
    bool isFlagSet(const std::string& flagName) const;
    std::vector<std::string> getInputFiles() const;
};

#endif
