#pragma once

#include <cstdint>
#include <string>

struct SystemConfig {
    uint64_t numCpu = 0;
    std::string scheduler;
    uint64_t quantumCycles = 0;
    uint64_t batchProcessFreq = 0;
    uint64_t minIns = 0;
    uint64_t maxIns = 0;
    uint64_t delaysPerExec = 0;
};

// Returns an error message, or "" if the file loaded.
std::string loadConfig(const std::string& path, SystemConfig& out);
