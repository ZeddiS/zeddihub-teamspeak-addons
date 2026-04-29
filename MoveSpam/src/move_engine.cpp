#include "move_engine.h"

#include <chrono>

#include "ts3_functions.h"
#include "teamspeak/public_errors.h"

namespace {
constexpr int kMinIntervalMs = 200;  // anti-flood floor
}

MoveEngine::MoveEngine(const TS3Functions* funcs) : fn_(funcs) {}

MoveEngine::~MoveEngine() {
    stop();
}

void MoveEngine::start(const MoveJob& job) {
    stop();
    job_ = job;
    if (job_.intervalMs < kMinIntervalMs) job_.intervalMs = kMinIntervalMs;
    if (job_.maxMoves < 0) job_.maxMoves = 0;
    sent_.store(0);
    stopRequested_.store(false);
    running_.store(true);
    t_ = std::thread([this] { worker(); });
}

void MoveEngine::stop() {
    {
        std::lock_guard<std::mutex> lk(mu_);
        stopRequested_.store(true);
    }
    cv_.notify_all();
    if (t_.joinable()) t_.join();
    running_.store(false);
}

void MoveEngine::worker() {
    bool toB = true;
    int consecutiveErrors = 0;

    for (int i = 0; ; ++i) {
        if (stopRequested_.load()) break;
        if (job_.maxMoves > 0 && i >= job_.maxMoves) break;

        const uint64 dest = toB ? job_.channelB : job_.channelA;
        toB = !toB;

        if (fn_ && fn_->requestClientMove) {
            unsigned int err = fn_->requestClientMove(
                job_.schid, job_.clientID, dest, "", nullptr);
            if (err == ERROR_ok) {
                sent_.fetch_add(1);
                consecutiveErrors = 0;
            } else {
                if (++consecutiveErrors >= 3) {
                    // Three failures in a row — give up.
                    break;
                }
            }
        }

        std::unique_lock<std::mutex> lk(mu_);
        cv_.wait_for(lk, std::chrono::milliseconds(job_.intervalMs),
                     [this] { return stopRequested_.load(); });
    }

    running_.store(false);
}
