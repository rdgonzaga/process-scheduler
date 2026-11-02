#include "core/Scheduler.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "Config.h"

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::start(const SystemConfig& config) {
    config_ = config;
    cores_.assign(config.numCpu, Core{});
    quit_ = false;
    quitting_ = false;
    sync_.emplace(static_cast<std::ptrdiff_t>(config.numCpu + 1));

    for (int i = 0; i < static_cast<int>(config.numCpu); ++i) coreThreads_.emplace_back(&Scheduler::coreLoop, this, i);
    clock_ = std::thread(&Scheduler::clockLoop, this);
}

void Scheduler::stop() {
    if (!clock_.joinable()) return;
    quit_ = true;
    clock_.join();
    for (std::thread& t : coreThreads_) t.join();
    coreThreads_.clear();
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
    std::lock_guard<std::mutex> lock(mtx);

    int used = 0;
    for (const Core& core : cores_) used += core.process ? 1 : 0;
    size_t width = 10;
    for (const auto& [name, p] : byName_) width = std::max(width, name.size());

    int total = static_cast<int>(cores_.size());
    std::ostringstream out;
    out << "CPU utilization: " << (total ? used * 100 / total : 0) << "%\n";
    out << "Cores used: " << used << "\n";
    out << "Cores available: " << total - used << "\n\n";
    out << "--------------------------------------\n";

    auto row = [&](const Process& p, const std::string& where, uint64_t line) {
        out << std::left << std::setw(static_cast<int>(width) + 3) << p.name
            << "(" << timestamp(p.createdAt) << ")    "
            << std::setw(12) << where
            << line << " / " << p.totalLines << "\n";
    };

    out << "Running processes:\n";
    for (int i = 0; i < total; ++i) {
        if (cores_[i].process) row(*cores_[i].process, "Core: " + std::to_string(i), cores_[i].process->pc);
    }
    // waiting for a core or asleep: still running as far as the user can tell
    for (const ProcessPtr& p : ready_) row(*p, "Core: -", p->pc);
    for (const Sleeper& s : sleeping_) row(*s.process, "Core: -", s.process->pc);
    out << "\nFinished processes:\n";
    for (const ProcessPtr& p : finished_) row(*p, "Finished", p->totalLines);
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

void Scheduler::clockLoop() {
    while (true) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            ++tick_;

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
        std::this_thread::sleep_for(std::chrono::milliseconds(config::TICK_MS));
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
            p.delayLeft = 0;
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
