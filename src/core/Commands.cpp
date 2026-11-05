#include "core/Commands.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "art/AsciiArt.h"
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
    shell.scheduler.start(shell.config);
    std::cout << "Initialized with " << shell.config.numCpu << " CPU(s) using " << shell.config.scheduler << ".\n";
}

void splitCommand(const std::string& line, std::string& name, std::string& args) {
    std::string trimmed = trim(line);
    size_t space = trimmed.find_first_of(" \t");
    name = toLower(trimmed.substr(0, space));
    args = space == std::string::npos ? "" : trim(trimmed.substr(space + 1));
}

struct Screen {
    Shell& shell;
    ProcessPtr process;
    bool detached = false;
};

struct ScreenCommand {
    const char* name;
    void (*run)(Screen& screen);
};

void cmdProcessSmi(Screen& screen) {
    std::string out;
    {
        std::lock_guard<std::mutex> lock(screen.shell.scheduler.mtx);
        const Process& p = *screen.process;
        std::string message = printMessage(p);

        out += "Process name: " + p.name + "\n";
        out += "ID: " + std::to_string(p.id) + "\n";
        out += "Logs:\n";
        for (const LogEntry& log : p.logs) {
            out += "(" + timestamp(log.time) + ") Core:" + std::to_string(log.core) + " \"" + message + "\"\n";
        }
        out += "\n";
        if (p.finished()) {
            out += "Finished!\n";
        } else {
            out += "Current instruction line: " + std::to_string(p.pc) + "\n";
            out += "Lines of code: " + std::to_string(p.totalLines) + "\n";
        }
    }
    console::write(out + "\n");
}

void cmdScreenExit(Screen& screen) {
    screen.detached = true;
}

const std::vector<ScreenCommand>& screenMenu() {
    static const std::vector<ScreenCommand> table = {
        {"process-smi", cmdProcessSmi},
        {"exit",        cmdScreenExit},
    };
    return table;
}

void attach(Shell& shell, ProcessPtr process) {
    console::clear();
    Screen screen{shell, std::move(process)};
    std::string line, name, args;

    while (!screen.detached) {
        std::cout << "root:\\> " << std::flush;
        if (!std::getline(std::cin, line)) {
            shell.quit = true;
            return;
        }
        splitCommand(line, name, args);
        if (name.empty()) continue;

        auto it = std::find_if(screenMenu().begin(), screenMenu().end(),
                               [&](const ScreenCommand& c) { return name == c.name; });
        if (it == screenMenu().end()) {
            console::error("Error: '" + name + "' is not a recognized command.");
            continue;
        }
        it->run(screen);
    }

    console::clear();
    console::write(ascii::header());
}

void cmdScreen(Shell& shell, const std::string& args) {
    std::string flag, name;
    splitCommand(args, flag, name);

    if (flag == "-ls" && name.empty()) {
        console::write(shell.scheduler.report() + "\n");
        return;
    }
    if ((flag != "-s" && flag != "-r") || name.empty()) {
        console::error("Usage: screen -s <name> | screen -r <name> | screen -ls");
        return;
    }

    ProcessPtr process = flag == "-s" ? shell.scheduler.spawn(name) : shell.scheduler.find(name);
    if (!process) {
        console::error(flag == "-s" ? "Process " + name + " already exists." : "Process " + name + " not found.");
        return;
    }
    attach(shell, process);
}

void cmdScreenLs(Shell& shell, const std::string& args) {
    if (!args.empty()) {
        console::error("Usage: screen-ls");
        return;
    }
    cmdScreen(shell, "-ls");
}

void cmdSchedulerStart(Shell& shell, const std::string&) {
    if (!shell.scheduler.startBatches()) {
        console::error("The scheduler is already generating processes.");
        return;
    }
    std::cout << "Scheduler started. Generating processes every " << shell.config.batchProcessFreq << " tick(s).\n";
}

void cmdSchedulerStop(Shell& shell, const std::string&) {
    if (!shell.scheduler.stopBatches()) {
        console::error("The scheduler isn't generating processes.");
        return;
    }
    std::cout << "Scheduler stopped generating processes.\n";
}

void cmdReportUtil(Shell& shell, const std::string&) {
    std::error_code ec;
    std::filesystem::path path = std::filesystem::absolute("csopesy-log.txt", ec);
    if (ec) path = "csopesy-log.txt";

    std::ofstream file(path);
    file << shell.scheduler.report();
    file.close();
    if (!file) {
        console::error("Error: couldn't write " + path.generic_string());
        return;
    }
    std::cout << "Report generated at " << path.generic_string() << "!\n";
}

void cmdExit(Shell& shell, const std::string&) {
    shell.quit = true;
}

const std::vector<Command>& mainMenu() {
    static const std::vector<Command> table = {
        {"initialize",      false, cmdInitialize},
        {"screen",          true,  cmdScreen},
        {"screen-ls",       true,  cmdScreenLs},
        {"scheduler-start", true,  cmdSchedulerStart},
        {"scheduler-test",  true,  cmdSchedulerStart},
        {"scheduler-stop",  true,  cmdSchedulerStop},
        {"report-util",     true,  cmdReportUtil},
        {"exit",            false, cmdExit},
    };
    return table;
}

}

void execute(Shell& shell, const std::string& line) {
    std::string name, args;
    splitCommand(line, name, args);
    if (name.empty()) return;

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
