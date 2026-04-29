#include "voice_engine.h"

#include <algorithm>
#include <cmath>

namespace {

float ratioFromSemitones(float st) {
    return std::pow(2.0f, st / 12.0f);
}

void int16ToFloat(const short* src, float* dst, int n) {
    constexpr float kScale = 1.0f / 32768.0f;
    for (int i = 0; i < n; ++i) dst[i] = (float)src[i] * kScale;
}

void floatToInt16(const float* src, short* dst, int n) {
    for (int i = 0; i < n; ++i) {
        float v = src[i] * 32768.0f;
        if (v > 32767.0f) v = 32767.0f;
        else if (v < -32768.0f) v = -32768.0f;
        dst[i] = (short)v;
    }
}

}  // namespace

VoiceEngine::VoiceEngine() {
    echoBuf_.assign((std::size_t)(kSampleRate * 0.5f), 0.0f);  // up to 500ms
}

void VoiceEngine::setConfig(const VoiceConfig& cfg) {
    std::lock_guard<std::mutex> lk(mu_);
    cfg_ = cfg;
    enabled_.store(cfg.enabled);
    pitchShifter_.reset();
    std::fill(echoBuf_.begin(), echoBuf_.end(), 0.0f);
    echoIdx_ = 0;
    robotPhase_ = 0.0f;
}

VoiceConfig VoiceEngine::config() const {
    std::lock_guard<std::mutex> lk(mu_);
    return cfg_;
}

bool VoiceEngine::processSamples(short* samples, int sampleCount, int channels) {
    if (!enabled_.load()) return false;
    if (!samples || sampleCount <= 0) return false;

    VoiceConfig cfg;
    {
        std::lock_guard<std::mutex> lk(mu_);
        cfg = cfg_;
    }
    if (cfg.preset == VoicePreset::Off) return false;

    // Mix to mono (TS3 capture is typically mono = 1 channel anyway)
    const int totalSamples = sampleCount * channels;

    floatScratchIn_.resize((std::size_t)totalSamples);
    floatScratchOut_.resize((std::size_t)totalSamples);

    int16ToFloat(samples, floatScratchIn_.data(), totalSamples);
    std::copy(floatScratchIn_.begin(), floatScratchIn_.end(),
              floatScratchOut_.begin());

    switch (cfg.preset) {
        case VoicePreset::Helium:
            applyPitchShift(floatScratchOut_.data(), totalSamples, ratioFromSemitones(6.0f));
            break;
        case VoicePreset::Chipmunk:
            applyPitchShift(floatScratchOut_.data(), totalSamples, ratioFromSemitones(12.0f));
            break;
        case VoicePreset::Demon:
            applyPitchShift(floatScratchOut_.data(), totalSamples, ratioFromSemitones(-7.0f));
            break;
        case VoicePreset::Deep:
            applyPitchShift(floatScratchOut_.data(), totalSamples, ratioFromSemitones(-4.0f));
            break;
        case VoicePreset::Custom:
            applyPitchShift(floatScratchOut_.data(), totalSamples,
                            ratioFromSemitones(cfg.customSemitones));
            break;
        case VoicePreset::Robot:
            applyRobot(floatScratchOut_.data(), totalSamples);
            break;
        case VoicePreset::Echo:
            applyEcho(floatScratchOut_.data(), totalSamples);
            break;
        case VoicePreset::Off:
        default:
            return false;
    }

    floatToInt16(floatScratchOut_.data(), samples, totalSamples);
    return true;
}

void VoiceEngine::applyPitchShift(float* buffer, int n, float ratio) {
    pitchShifter_.process(ratio, n, kSampleRate, buffer, buffer);
}

void VoiceEngine::applyRobot(float* buffer, int n) {
    const float dt = 1.0f / kSampleRate;
    const float twoPi = 6.2831853f;
    const float hz = cfg_.robotHz > 0.5f ? cfg_.robotHz : 30.0f;
    for (int i = 0; i < n; ++i) {
        float carrier = std::sin(twoPi * hz * (robotPhase_ * dt));
        buffer[i] *= carrier;
        robotPhase_ += 1.0f;
        if (robotPhase_ > kSampleRate) robotPhase_ -= kSampleRate;
    }
}

void VoiceEngine::applyEcho(float* buffer, int n) {
    const std::size_t delaySamples = (std::size_t)(cfg_.echoDelayMs * kSampleRate / 1000.0f);
    const std::size_t bufSize = echoBuf_.size();
    const float fb = std::clamp(cfg_.echoFeedback, 0.0f, 0.95f);

    for (int i = 0; i < n; ++i) {
        std::size_t readIdx = (echoIdx_ + bufSize - delaySamples) % bufSize;
        float delayed = echoBuf_[readIdx];
        float out = buffer[i] + delayed * 0.7f;
        echoBuf_[echoIdx_] = buffer[i] + delayed * fb;
        echoIdx_ = (echoIdx_ + 1) % bufSize;
        buffer[i] = out;
    }
}
