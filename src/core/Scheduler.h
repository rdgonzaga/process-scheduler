#pragma once

#include <atomic>
#include <barrier>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "core/Process.h"
#include "core/SystemConfig.h"

using ProcessPtr = std::shared_ptr<Process>;

// One clock thread plus one thread per core. Every tick the clock hands out
// processes, then the cores each run one tick in lockstep on a barrier.
class Scheduler {
public:
    ~Scheduler();

    void start(const SystemConfig& config);
    void stop();

    // nullptr if the name is taken
    ProcessPtr spawn(const std::string& name);
    // nullptr if there's no such process or it already finished
    ProcessPtr find(const std::string& name);

    // false if batches were already on / already off
    bool startBatches();
    bool stopBatches();

    // the screen -ls text: utilization, then running and finished processes
    std::string report();

    // guards everything below, and any Process reachable from it
    std::mutex mtx;

private:
    struct Core {
        ProcessPtr process;
        uint64_t ticksUsed = 0;
    };

    struct Sleeper {
        uint64_t wakeTick;
        ProcessPtr process;
    };

    // call with mtx held
    void fillCores();
    void clockLoop();
    void coreLoop(int index);
    void runCore(int index);

    SystemConfig config_;
    uint64_t tick_ = 0;
    int nextId_ = 1;
    std::vector<Core> cores_;
    std::deque<ProcessPtr> ready_;
    std::vector<Sleeper> sleeping_;
    std::vector<ProcessPtr> finished_;
    std::map<std::string, ProcessPtr> byName_;

    // clock thread only
    uint64_t sinceBatch_ = 0;
    int batchNo_ = 0;

    std::atomic<bool> generating_ = false;
    std::atomic<bool> quit_ = false;
    bool quitting_ = false;
    std::optional<std::barrier<>> sync_;
    std::thread clock_;
    std::vector<std::thread> coreThreads_;
};
