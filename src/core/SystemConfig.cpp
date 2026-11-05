#include "core/SystemConfig.h"

#include <charconv>
#include <fstream>
#include <map>

namespace {

constexpr uint64_t MAX_VALUE = 1ull << 32;

struct Range {
    uint64_t SystemConfig::*field;
    uint64_t min;
    uint64_t max;
};

const std::map<std::string, Range> NUMBER_KEYS = {
    {"num-cpu",            {&SystemConfig::numCpu, 1, 128}},
    {"quantum-cycles",     {&SystemConfig::quantumCycles, 1, MAX_VALUE}},
    {"batch-process-freq", {&SystemConfig::batchProcessFreq, 1, MAX_VALUE}},
    {"min-ins",            {&SystemConfig::minIns, 1, MAX_VALUE}},
    {"max-ins",            {&SystemConfig::maxIns, 1, MAX_VALUE}},
    {"delays-per-exec",    {&SystemConfig::delaysPerExec, 0, MAX_VALUE}},
};

std::string unquote(const std::string& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') return s.substr(1, s.size() - 2);
    return s;
}

}

std::string loadConfig(const std::string& path, SystemConfig& out) {
    std::ifstream file(path);
    if (!file) return "Could not open " + path + ".";

    SystemConfig loaded;
    std::map<std::string, bool> seen;
    std::string key, value;

    while (file >> key) {
        if (!(file >> value)) return "Missing value for '" + key + "' in " + path + ".";
        value = unquote(value);

        // the spec's sample spells it this way
        if (key == "delay-per-exec") key = "delays-per-exec";

        if (key == "scheduler") {
            if (value != "fcfs" && value != "rr") return "scheduler must be \"fcfs\" or \"rr\".";
            loaded.scheduler = value;
            seen[key] = true;
            continue;
        }

        if (key == "x-counter") {
            if (value != "0" && value != "1") return "x-counter must be 0 or 1.";
            loaded.xCounter = value == "1";
            continue;
        }

        auto it = NUMBER_KEYS.find(key);
        if (it == NUMBER_KEYS.end()) continue;

        const Range& range = it->second;
        uint64_t number = 0;
        auto [end, ec] = std::from_chars(value.data(), value.data() + value.size(), number);
        if (ec != std::errc() || end != value.data() + value.size() || number < range.min || number > range.max) {
            return key + " must be a whole number from " + std::to_string(range.min) + " to " +
                   std::to_string(range.max) + ".";
        }
        loaded.*range.field = number;
        seen[key] = true;
    }

    if (!seen["scheduler"]) return "Missing 'scheduler' in " + path + ".";
    for (const auto& [name, range] : NUMBER_KEYS) {
        if (!seen[name]) return "Missing '" + name + "' in " + path + ".";
    }
    if (loaded.minIns > loaded.maxIns) return "min-ins can't be bigger than max-ins.";

    out = loaded;
    return "";
}
