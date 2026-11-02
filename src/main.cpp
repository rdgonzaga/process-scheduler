#include <iostream>
#include <string>

#include "art/AsciiArt.h"
#include "core/Commands.h"
#include "ui/Console.h"

int main() {
    console::init();
    console::write(ascii::header());

    Shell shell;
    std::string line;
    while (!shell.quit) {
        std::cout << "root:\\> " << std::flush;
        if (!std::getline(std::cin, line)) break;
        commands::execute(shell, line);
    }

    console::restore();
    return 0;
}
