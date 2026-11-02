#pragma once

#include <string>

#include "core/SystemConfig.h"

struct Shell {
    bool quit = false;
    bool initialized = false;
    SystemConfig config;
};

namespace commands {

struct Command {
    const char* name;
    bool needsInit;
    void (*run)(Shell& shell, const std::string& args);
};

void execute(Shell& shell, const std::string& line);

}
