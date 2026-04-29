#include "poke_engine.h"

#include <algorithm>
#include <chrono>
#include <random>

#include "ts3_functions.h"

namespace {
constexpr int kMinDelayMs = 50;
constexpr int kHardCapPokes = 500;
}

PokeEngine::PokeEngine(const TS3Functions* funcs) : fn_(funcs) {}

PokeEngine::~PokeEngine() {
    stop();
}

void PokeEngine::start(const PokeJob& job) {
    stop();

    job_ = job;
    if (job_.messages.empty()) {
        job_.messages.push_back(" ");
    }
    if (job_.count <= 0) job_.count = 1;
    if (job_.count > kHardCapPokes) job_.count = kHardCapPokes;
    if (job_.intervalMaxMs < job_.intervalMinMs) {
        job_.intervalMaxMs = job_.intervalMinMs;
    }

    sent_.store(0);
    total_.store(job_.count);
    stopRequested_.store(false);
    running_.store(true);
    t_ = std::thread([this] { worker(); });
}

void PokeEngine::stop() {
    {
        std::lock_guard<std::mutex> lk(mu_);
        stopRequested_.store(true);
    }
    cv_.notify_all();
    if (t_.joinable()) t_.join();
    running_.store(false);
}

void PokeEngine::worker() {
    std::mt19937 rng(static_cast<unsigned>(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    std::uniform_int_distribution<int> dist(
        job_.intervalMinMs, std::max(job_.intervalMinMs, job_.intervalMaxMs));

    for (int i = 0; i < job_.count; ++i) {
        if (stopRequested_.load()) break;

        const std::string& msg = job_.messages[i % job_.messages.size()];
        if (fn_ && fn_->requestClientPoke) {
            fn_->requestClientPoke(job_.schid, job_.clientID, msg.c_str(), nullptr);
        }
        sent_.fetch_add(1);

        if (i + 1 >= job_.count) break;

        int waitMs = (job_.mode == PokeMode::Burst)
                         ? job_.burstDelayMs
                         : dist(rng);
        if (waitMs < kMinDelayMs) waitMs = kMinDelayMs;

        std::unique_lock<std::mutex> lk(mu_);
        cv_.wait_for(lk, std::chrono::milliseconds(waitMs),
                     [this] { return stopRequested_.load(); });
    }

    running_.store(false);
}
