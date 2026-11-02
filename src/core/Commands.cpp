#include "core/Commands.h"

#include <cctype>
#include <iostream>
#include <vector>

#include "ui/Console.h"

namespace commands {

namespace {

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t");
    if (start == std::string::npos) return "";
    return s.substr(start, s.find_last_not_of(" \t") - start + 1);
}

std::string toLower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

void cmdInitialize(Shell& shell, const std::string&) {
    if (shell.initialized) {
        console::error("The system is already initialized.");
        return;
    }
    std::string problem = loadConfig("config.txt", shell.config);
    if (!problem.empty()) {
        console::error("Error: " + problem);
        return;
    }
    shell.initialized = true;
    std::cout << "Initialized with " << shell.config.numCpu << " CPU(s) using " << shell.config.scheduler << ".\n";
}

void cmdExit(Shell& shell, const std::string&) {
    shell.quit = true;
}

const std::vector<Command>& mainMenu() {
    static const std::vector<Command> table = {
        {"initialize", false, cmdInitialize},
        {"exit",       false, cmdExit},
    };
    return table;
}

}

void execute(Shell& shell, const std::string& line) {
    std::string trimmed = trim(line);
    if (trimmed.empty()) return;

    size_t space = trimmed.find(' ');
    std::string name = toLower(trimmed.substr(0, space));
    std::string args = space == std::string::npos ? "" : trim(trimmed.substr(space + 1));

    for (const Command& command : mainMenu()) {
        if (name == command.name) {
            if (command.needsInit && !shell.initialized) {
                console::error("Error: run 'initialize' first.");
                return;
            }
            command.run(shell, args);
            return;
        }
    }

    console::error("Error: '" + name + "' is not a recognized command.");
}

}
