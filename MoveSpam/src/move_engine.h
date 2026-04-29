#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

struct TS3Functions;
using anyID = unsigned short;
using uint64 = unsigned long long;

struct MoveJob {
    uint64 schid = 0;
    anyID clientID = 0;
    uint64 channelA = 0;       // first toggle target (e.g. user's current channel)
    uint64 channelB = 0;       // second toggle target (e.g. default channel)
    int intervalMs = 1500;     // delay between moves
    int maxMoves = 200;        // safety cap (0 = unlimited until STOP)
};

class MoveEngine {
public:
    explicit MoveEngine(const TS3Functions* funcs);
    ~MoveEngine();

    MoveEngine(const MoveEngine&) = delete;
    MoveEngine& operator=(const MoveEngine&) = delete;

    void start(const MoveJob& job);
    void stop();

    bool isRunning() const { return running_.load(); }
    int sent() const { return sent_.load(); }

private:
    void worker();

    const TS3Functions* fn_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stopRequested_{false};
    std::atomic<int> sent_{0};
    std::thread t_;
    std::mutex mu_;
    std::condition_variable cv_;
    MoveJob job_;
};
