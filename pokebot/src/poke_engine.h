#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct TS3Functions;

using anyID = unsigned short;
using uint64 = unsigned long long;

enum class PokeMode { Burst, Schedule };

struct PokeJob {
    uint64 schid = 0;
    anyID clientID = 0;
    PokeMode mode = PokeMode::Schedule;
    std::vector<std::string> messages;
    int count = 10;
    int intervalMinMs = 3000;
    int intervalMaxMs = 5000;
    int burstDelayMs = 200;
    std::string label;
};

class PokeEngine {
public:
    explicit PokeEngine(const TS3Functions* funcs);
    ~PokeEngine();

    PokeEngine(const PokeEngine&) = delete;
    PokeEngine& operator=(const PokeEngine&) = delete;

    void start(const PokeJob& job);
    void stop();

    bool isRunning() const { return running_.load(); }
    int sent() const { return sent_.load(); }
    int total() const { return total_.load(); }

private:
    void worker();

    const TS3Functions* fn_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stopRequested_{false};
    std::atomic<int> sent_{0};
    std::atomic<int> total_{0};
    std::thread t_;
    std::mutex mu_;
    std::condition_variable cv_;
    PokeJob job_;
};
