#include "ui/Console.h"

#include <iostream>

#include "Config.h"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace console {

namespace {

DWORD originalMode = 0;
UINT originalCodePage = 0;

}

void init() {
    originalCodePage = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);

    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(out, &originalMode);
    SetConsoleMode(out, originalMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT);
}

void restore() {
    write(RESET);
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), originalMode);
    SetConsoleOutputCP(originalCodePage);
}

// cout writes byte by byte, which can split the logo's UTF-8 blocks in conhost
void write(const std::string& text) {
    std::cout.flush();
    DWORD written = 0;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), text.data(), static_cast<DWORD>(text.size()), &written, nullptr);
}

void clear() {
    write("\x1b[2J\x1b[3J\x1b[H");
}

void error(const std::string& message) {
    write(fg(config::COLOR_ERROR) + message + RESET + "\n");
}

std::string fg(int color) { return "\x1b[38;5;" + std::to_string(color) + "m"; }

}
#else
namespace console {

void init() {}

void restore() {
    write(RESET);
}

void write(const std::string& text) {
    std::cout << text << std::flush;
}

void clear() {
    write("\x1b[2J\x1b[3J\x1b[H");
}

void error(const std::string& message) {
    write(fg(config::COLOR_ERROR) + message + RESET + "\n");
}

std::string fg(int color) { return "\x1b[38;5;" + std::to_string(color) + "m"; }

}
#endif
