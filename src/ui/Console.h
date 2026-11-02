#pragma once

#include <string>

namespace console {

void init();
void restore();

void write(const std::string& text);
void clear();
void error(const std::string& message);

std::string fg(int color);

inline constexpr const char* RESET = "\x1b[0m";
inline constexpr const char* BOLD  = "\x1b[1m";

}
