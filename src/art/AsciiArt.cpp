#include "art/AsciiArt.h"

#include <algorithm>
#include <sstream>
#include <vector>

#include "Config.h"
#include "ui/Console.h"

namespace ascii {

const char* const LOGO = R"(  █████████   █████████     ███████    ███████████  ██████████  █████████  █████ █████
  ███░░░░░███ ███░░░░░███  ███░░░░░███ ░░███░░░░░███░░███░░░░░█ ███░░░░░███░░███ ░░███
 ███     ░░░ ░███    ░░░  ███     ░░███ ░███    ░███ ░███  █ ░ ░███    ░░░  ░░███ ███
░███         ░░█████████ ░███      ░███ ░██████████  ░██████   ░░█████████   ░░█████
░███          ░░░░░░░░███░███      ░███ ░███░░░░░░   ░███░░█    ░░░░░░░░███   ░░███
░░███     ███ ███    ░███░░███     ███  ░███         ░███ ░   █ ███    ░███    ░███
 ░░█████████ ░░█████████  ░░░███████░   █████        ██████████░░█████████     █████
  ░░░░░░░░░   ░░░░░░░░░     ░░░░░░░    ░░░░░        ░░░░░░░░░░  ░░░░░░░░░     ░░░░░)";

const char* const INFO = R"(-----------------------------------------
Welcome to CSOPESY Emulator!

Developers: (Group 6, S01)
Gonzaga, Rainer
Gonzales, Aaron James
Ramos, Richmond Jose
Yasumuro, Mariel

Last updated: 11-02-2026
-----------------------------------------)";

std::string header() {
    std::vector<std::string> rows;
    std::istringstream in(LOGO);
    for (std::string line; std::getline(in, line);) rows.push_back(line);

    const int shades = static_cast<int>(std::size(config::LOGO_GRADIENT));
    std::string out = console::BOLD;
    for (size_t r = 0; r < rows.size(); ++r) {
        int shade = std::min(shades - 1, static_cast<int>(r * shades / rows.size()));
        out += console::fg(config::LOGO_GRADIENT[shade]) + rows[r] + "\n";
    }
    out += console::RESET;
    out += console::fg(config::COLOR_INFO) + INFO + console::RESET + "\n\n";
    return out;
}

}
