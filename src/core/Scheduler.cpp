#include "core/Scheduler.h"

#include <algorithm>
#include <chrono>

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

void Scheduler::clockLoop() {
    while (true) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            ++tick_;

            auto awake = std::stable_partition(sleeping_.begin(), sleeping_.end(),
                                               [this](const Sleeper& s) { return s.wakeTick > tick_; });
            for (auto it = awake; it != sleeping_.end(); ++it) ready_.push_back(it->process);
            sleeping_.erase(awake, sleeping_.end());

            for (Core& core : cores_) {
                if (core.process || ready_.empty()) continue;
                core.process = ready_.front();
                core.delayLeft = 0;
                ready_.pop_front();
            }
        }

        // cores read quitting_ only after this wait, so they all agree with the clock
        quitting_ = quit_;
        sync_->arrive_and_wait();
        if (quitting_) return;
        sync_->arrive_and_wait();

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

    if (core.delayLeft > 0) {
        --core.delayLeft;
        return;
    }

    uint8_t sleepTicks = step(*core.process, index);
    core.delayLeft = config_.delaysPerExec;

    if (core.process->finished()) {
        finished_.push_back(core.process);
        core.process = nullptr;
    } else if (sleepTicks > 0) {
        sleeping_.push_back({tick_ + sleepTicks, core.process});
        core.process = nullptr;
    }
}
