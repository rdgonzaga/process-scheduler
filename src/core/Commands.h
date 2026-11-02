#pragma once

#include <string>

struct Shell {
    bool quit = false;
};

namespace commands {

struct Command {
    const char* name;
    void (*run)(Shell& shell, const std::string& args);
};

void execute(Shell& shell, const std::string& line);

}
