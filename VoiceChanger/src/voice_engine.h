#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

#include "../vendor/smb_pitch_shift.h"

enum class VoicePreset : int {
    Off       = 0,
    Helium    = 1,   // pitch +6 semitones
    Chipmunk  = 2,   // pitch +12 semitones
    Demon     = 3,   // pitch -7 semitones
    Deep      = 4,   // pitch -4 semitones
    Robot     = 5,   // ring modulation
    Echo      = 6,   // delay buffer
    Custom    = 7,   // user pitch slider
};

struct VoiceConfig {
    VoicePreset preset = VoicePreset::Off;
    float customSemitones = 0.0f;     // -12 .. +12
    float echoDelayMs = 200.0f;
    float echoFeedback = 0.30f;
    float robotHz = 30.0f;
    bool enabled = false;
};

class VoiceEngine {
public:
    VoiceEngine();

    void setConfig(const VoiceConfig& cfg);
    VoiceConfig config() const;

    bool enabled() const { return enabled_.load(); }
    void setEnabled(bool e) { enabled_.store(e); }

    // TS3 captured-voice callback. Mutates samples in place.
    // Returns true if samples were modified.
    bool processSamples(short* samples, int sampleCount, int channels);

private:
    void applyPitchShift(float* buffer, int n, float ratio);
    void applyRobot(float* buffer, int n);
    void applyEcho(float* buffer, int n);

    mutable std::mutex mu_;
    VoiceConfig cfg_;
    std::atomic<bool> enabled_{false};

    pitchshift::PitchShifter pitchShifter_;
    std::vector<float> floatScratchIn_;
    std::vector<float> floatScratchOut_;

    // Echo state
    std::vector<float> echoBuf_;
    std::size_t echoIdx_ = 0;

    // Robot state
    float robotPhase_ = 0.0f;

    static constexpr float kSampleRate = 48000.0f;
};
