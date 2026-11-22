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
    bool xCounter = false;
    uint64_t maxOverallMem = 0;   // total memory, bytes
    uint64_t memPerFrame = 0;     
    uint64_t minMemPerProc = 0;
    uint64_t maxMemPerProc = 0;
};

// Returns an error message, or "" if the file loaded.
std::string loadConfig(const std::string& path, SystemConfig& out);
