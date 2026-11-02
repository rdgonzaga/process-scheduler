#include "core/Scheduler.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "Config.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::start(const SystemConfig& config) {
    config_ = config;
    cores_.assign(config.numCpu, Core{});
    quit_ = false;
    quitting_ = false;
    sync_.emplace(static_cast<std::ptrdiff_t>(config.numCpu + 1));

    // Windows rounds sleep_for up to ~15.6 ms by default, which would stretch a 10 ms tick
    timeBeginPeriod(1);

    for (int i = 0; i < static_cast<int>(config.numCpu); ++i) coreThreads_.emplace_back(&Scheduler::coreLoop, this, i);
    clock_ = std::thread(&Scheduler::clockLoop, this);
}

void Scheduler::stop() {
    if (!clock_.joinable()) return;
    quit_ = true;
    clock_.join();
    for (std::thread& t : coreThreads_) t.join();
    coreThreads_.clear();
    timeEndPeriod(1);
}

ProcessPtr Scheduler::spawn(const std::string& name) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (byName_.count(name)) return nullptr;
    }

    // generating thousands of instructions shouldn't stall the cores
    auto process = std::make_shared<Process>(makeProcess(0, name, config_.minIns, config_.maxIns));

    std::lock_guard<std::mutex> lock(mtx);
    if (byName_.count(name)) return nullptr;
    process->id = nextId_++;
    byName_[name] = process;
    ready_.push_back(process);
    return process;
}

ProcessPtr Scheduler::find(const std::string& name) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = byName_.find(name);
    if (it == byName_.end() || it->second->finished()) return nullptr;
    return it->second;
}

std::string Scheduler::report() {
    struct Row {
        std::string name;
        std::time_t createdAt;
        int core;
        uint64_t line;
        uint64_t totalLines;
    };
    std::vector<Row> running, finished;
    int used = 0;
    int total = 0;

    // copy under the lock and format after, so a long list doesn't stall the cores
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto add = [](std::vector<Row>& rows, const Process& p, int core, uint64_t line) {
            rows.push_back({p.name, p.createdAt, core, line, p.totalLines});
        };

        total = static_cast<int>(cores_.size());
        for (int i = 0; i < total; ++i) {
            if (!cores_[i].process) continue;
            ++used;
            add(running, *cores_[i].process, i, cores_[i].process->pc);
        }
        // waiting for a core or asleep: still running as far as the user can tell
        for (const ProcessPtr& p : ready_) add(running, *p, -1, p->pc);
        for (const Sleeper& s : sleeping_) add(running, *s.process, -1, s.process->pc);
        for (const ProcessPtr& p : finished_) add(finished, *p, -1, p->totalLines);
    }

    size_t width = 10;
    for (const Row& r : running) width = std::max(width, r.name.size());
    for (const Row& r : finished) width = std::max(width, r.name.size());

    std::ostringstream out;
    out << "CPU utilization: " << (total ? used * 100 / total : 0) << "%\n";
    out << "Cores used: " << used << "\n";
    out << "Cores available: " << total - used << "\n\n";
    out << "--------------------------------------\n";

    auto print = [&](const Row& r, const std::string& where) {
        out << std::left << std::setw(static_cast<int>(width) + 3) << r.name
            << "(" << timestamp(r.createdAt) << ")    "
            << std::setw(12) << where
            << r.line << " / " << r.totalLines << "\n";
    };

    out << "Running processes:\n";
    for (const Row& r : running) print(r, r.core < 0 ? "Core: -" : "Core: " + std::to_string(r.core));
    out << "\nFinished processes:\n";
    for (const Row& r : finished) print(r, "Finished");
    out << "--------------------------------------\n";
    return out.str();
}

void Scheduler::fillCores() {
    for (Core& core : cores_) {
        if (core.process || ready_.empty()) continue;
        core.process = ready_.front();
        core.ticksUsed = 0;
        ready_.pop_front();
    }
}

bool Scheduler::startBatches() {
    if (generating_) return false;
    sinceBatch_ = 0;
    generating_ = true;
    return true;
}

bool Scheduler::stopBatches() {
    return generating_.exchange(false);
}

void Scheduler::clockLoop() {
    const auto tickLength = std::chrono::milliseconds(config::TICK_MS);
    auto nextTick = std::chrono::steady_clock::now();
    while (true) {
        ProcessPtr batch;
        if (generating_ && ++sinceBatch_ >= config_.batchProcessFreq) {
            sinceBatch_ = 0;
            batch = std::make_shared<Process>(makeProcess(0, "", config_.minIns, config_.maxIns));
        }

        {
            std::lock_guard<std::mutex> lock(mtx);
            ++tick_;

            if (batch) {
                // skip names already taken with screen -s
                do {
                    ++batchNo_;
                    batch->name = (batchNo_ < 10 ? "p0" : "p") + std::to_string(batchNo_);
                } while (byName_.count(batch->name));
                batch->id = nextId_++;
                byName_[batch->name] = batch;
                ready_.push_back(batch);
            }

            auto awake = std::stable_partition(sleeping_.begin(), sleeping_.end(),
                                               [this](const Sleeper& s) { return s.wakeTick > tick_; });
            for (auto it = awake; it != sleeping_.end(); ++it) ready_.push_back(it->process);
            sleeping_.erase(awake, sleeping_.end());
            fillCores();
        }

        // cores read quitting_ only after this wait, so they all agree with the clock
        quitting_ = quit_;
        sync_->arrive_and_wait();
        if (quitting_) return;
        sync_->arrive_and_wait();

        // refill cores freed this tick, or screen -ls between ticks sees them idle
        {
            std::lock_guard<std::mutex> lock(mtx);
            fillCores();
        }
        // sleep to a fixed deadline so the tick's own work doesn't stretch it;
        // after a long stall, start over instead of rushing to catch up
        nextTick += tickLength;
        auto now = std::chrono::steady_clock::now();
        if (nextTick < now) nextTick = now;
        std::this_thread::sleep_until(nextTick);
    }
}

void Scheduler::coreLoop(int index) {
    while (true) {
        sync_->arrive_and_wait();
        if (quitting_) return;
        runCore(index);
        sync_->arrive_and_wait();
    }
}

void Scheduler::runCore(int index) {
    std::lock_guard<std::mutex> lock(mtx);
    Core& core = cores_[index];
    if (!core.process) return;

    Process& p = *core.process;
    ++core.ticksUsed;

    if (p.delayLeft > 0) {
        --p.delayLeft;
    } else {
        uint8_t sleepTicks = step(p, index);
        p.delayLeft = config_.delaysPerExec;

        if (p.finished()) {
            finished_.push_back(core.process);
            core.process = nullptr;
            return;
        }
        if (sleepTicks > 0) {
            sleeping_.push_back({tick_ + sleepTicks, core.process});
            core.process = nullptr;
            return;
        }
    }

    if (config_.scheduler == "rr" && core.ticksUsed >= config_.quantumCycles) {
        ready_.push_back(core.process);
        core.process = nullptr;
    }
}
