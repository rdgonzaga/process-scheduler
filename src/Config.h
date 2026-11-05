#pragma once

#include <cstdint>

namespace config {

inline constexpr int TICK_MS = 10;
inline constexpr uint64_t TICKS_PER_BATCH_TICK = 10;

// xterm 256-color codes, same as marquee-console
inline constexpr int LOGO_GRADIENT[] = {157, 120, 84, 78, 41, 35};
inline constexpr int COLOR_INFO  = 151;
inline constexpr int COLOR_ERROR = 203;

}
