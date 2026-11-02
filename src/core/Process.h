#pragma once

#include <array>
#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

inline constexpr int VAR_COUNT = 8;
inline constexpr uint8_t LITERAL = 0xFF;

enum class OpCode : uint8_t { Print, Declare, Add, Subtract, Sleep };

// Kept small on purpose: batch-process-freq 1 queues thousands of these.
struct Instruction {
    OpCode op;
    uint8_t target;
    uint8_t lhsVar;
    uint8_t rhsVar;
    uint16_t lhs;
    uint16_t rhs;
};

struct LogEntry {
    std::time_t time;
    int core;
};

struct Process {
    int id = 0;
    std::string name;
    std::time_t createdAt = 0;

    std::vector<Instruction> code;
    uint64_t pc = 0;
    uint64_t totalLines = 0;
    // busy-wait ticks left before the next instruction; survives preemption
    uint64_t delayLeft = 0;
    std::array<uint16_t, VAR_COUNT> vars{};
    std::vector<LogEntry> logs;

    bool finished() const { return pc >= totalLines; }
};

// FOR loops are unrolled here, so code.size() lands exactly in [minIns, maxIns].
Process makeProcess(int id, const std::string& name, uint64_t minIns, uint64_t maxIns);

// returns ticks to sleep, 0 if none
uint8_t step(Process& p, int core);

std::string printMessage(const Process& p);

// MM/DD/YYYY hh:mm:ssAM, the format in the spec's mockups
std::string timestamp(std::time_t time);
