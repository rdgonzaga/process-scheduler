#include "core/Commands.h"

#include <cctype>
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

void cmdExit(Shell& shell, const std::string&) {
    shell.quit = true;
}

const std::vector<Command>& mainMenu() {
    static const std::vector<Command> table = {
        {"exit", cmdExit},
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
            command.run(shell, args);
            return;
        }
    }

    console::error("Error: '" + name + "' is not a recognized command.");
}

}
