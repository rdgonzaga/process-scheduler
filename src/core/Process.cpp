#include "core/Process.h"

#include <algorithm>
#include <random>

namespace {

constexpr int MAX_FOR_DEPTH = 3;

std::mt19937_64& rng() {
    thread_local std::mt19937_64 engine{std::random_device{}()};
    return engine;
}

uint64_t randomBetween(uint64_t lo, uint64_t hi) {
    return std::uniform_int_distribution<uint64_t>(lo, hi)(rng());
}

uint8_t randomVar() { return static_cast<uint8_t>(randomBetween(0, VAR_COUNT - 1)); }

uint16_t randomValue() { return static_cast<uint16_t>(randomBetween(0, 100)); }

Instruction randomArithmetic(OpCode op) {
    Instruction ins{op, randomVar(), LITERAL, LITERAL, randomValue(), randomValue()};
    if (randomBetween(0, 1)) ins.lhsVar = randomVar();
    if (randomBetween(0, 1)) ins.rhsVar = randomVar();
    return ins;
}

Instruction randomInstruction() {
    switch (randomBetween(0, 4)) {
    case 0:  return {OpCode::Print, 0, LITERAL, LITERAL, 0, 0};
    case 1:  return {OpCode::Declare, randomVar(), LITERAL, LITERAL, randomValue(), 0};
    case 2:  return randomArithmetic(OpCode::Add);
    case 3:  return randomArithmetic(OpCode::Subtract);
    default: return {OpCode::Sleep, 0, LITERAL, LITERAL, static_cast<uint16_t>(randomBetween(1, 10)), 0};
    }
}

// Appends exactly `count` instructions, some of them unrolled FOR loops.
void generate(std::vector<Instruction>& out, uint64_t count, int depth) {
    while (count > 0) {
        bool makeLoop = depth < MAX_FOR_DEPTH && count >= 2 && randomBetween(0, 9) == 0;
        if (!makeLoop) {
            out.push_back(randomInstruction());
            --count;
            continue;
        }

        uint64_t repeats = randomBetween(2, std::min<uint64_t>(5, count));
        uint64_t bodySize = randomBetween(1, std::min<uint64_t>(10, count / repeats));

        std::vector<Instruction> body;
        generate(body, bodySize, depth + 1);
        for (uint64_t r = 0; r < repeats; ++r) out.insert(out.end(), body.begin(), body.end());
        count -= bodySize * repeats;
    }
}

uint16_t operand(const Process& p, uint8_t var, uint16_t literal) {
    return var == LITERAL ? literal : p.vars[var];
}

}

Process makeProcess(int id, const std::string& name, uint64_t minIns, uint64_t maxIns) {
    Process p;
    p.id = id;
    p.name = name;
    p.createdAt = std::time(nullptr);
    p.totalLines = randomBetween(minIns, maxIns);
    p.code.reserve(p.totalLines);
    generate(p.code, p.totalLines, 0);
    return p;
}

uint8_t step(Process& p, int core) {
    const Instruction& ins = p.code[p.pc];
    uint8_t sleepTicks = 0;

    switch (ins.op) {
    case OpCode::Print:
        p.logs.push_back({std::time(nullptr), core});
        break;
    case OpCode::Declare:
        p.vars[ins.target] = ins.lhs;
        break;
    case OpCode::Add: {
        int sum = operand(p, ins.lhsVar, ins.lhs) + operand(p, ins.rhsVar, ins.rhs);
        p.vars[ins.target] = static_cast<uint16_t>(std::min(sum, 65535));
        break;
    }
    case OpCode::Subtract: {
        int diff = operand(p, ins.lhsVar, ins.lhs) - operand(p, ins.rhsVar, ins.rhs);
        p.vars[ins.target] = static_cast<uint16_t>(std::max(diff, 0));
        break;
    }
    case OpCode::Sleep:
        sleepTicks = static_cast<uint8_t>(ins.lhs);
        break;
    }

    if (++p.pc >= p.totalLines) {
        p.code.clear();
        p.code.shrink_to_fit();
    }
    return sleepTicks;
}

std::string printMessage(const Process& p) {
    return "Hello world from " + p.name + "!";
}

std::string timestamp(std::time_t time) {
    std::tm local{};
    localtime_s(&local, &time);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%m/%d/%Y %I:%M:%S%p", &local);
    return buffer;
}
